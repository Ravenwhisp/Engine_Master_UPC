#include "Globals.h"
#include "VideoPlayback.h"

extern "C"
{
#include <libavutil/error.h>
}

namespace
{
	void printFFmpegError(const char* message, int errorCode, const std::filesystem::path& path)
	{
		char buffer[AV_ERROR_MAX_STRING_SIZE];
		av_strerror(errorCode, buffer, sizeof(buffer));

		DEBUG_LOG("[Video playback|%s] %s: %s", path.string().c_str(), message, buffer);
	}
}

VideoPlayback::VideoPlayback() = default;

VideoPlayback::~VideoPlayback()
{
	unload();
}

bool VideoPlayback::load(const std::filesystem::path& path)
{
	unload();

	if (path.empty())
		return false;

	m_path = path;

	const std::string pathString = path.string();

	int result = avformat_open_input(&m_formatContext, pathString.c_str(), nullptr, nullptr);

	if (result < 0)
	{
		printFFmpegError("Could not open video", result, m_path);
		unload();
		return false;
	}

	result = avformat_find_stream_info(m_formatContext, nullptr);

	if (result < 0)
	{
		printFFmpegError("Could not read stream information", result, m_path);
		unload();
		return false;
	}

	m_videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	m_audioStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

	if (m_videoStreamIndex < 0)
	{
		DEBUG_LOG("[Video playback|%s] No video stream found", m_path.string().c_str());
		unload();
		return false;
	}

	if (!openVideoDecoder())
	{
		unload();
		return false;
	}

	if (m_audioStreamIndex >= 0 && !openAudioDecoder())
	{
		unload();
		return false;
	}

	m_packet = av_packet_alloc();
	m_videoFrame = av_frame_alloc();

	if (m_audioStreamIndex >= 0)
		m_audioFrame = av_frame_alloc();

	if (!m_packet || !m_videoFrame || (m_audioStreamIndex >= 0 && !m_audioFrame))
	{
		DEBUG_LOG("[Video playback|%s] Could not allocate FFmpeg packet/frame", m_path.string().c_str());
		unload();
		return false;
	}

	if (m_formatContext->duration != AV_NOPTS_VALUE)
		m_duration = static_cast<double>(m_formatContext->duration) / static_cast<double>(AV_TIME_BASE);

	m_loaded = true;
	m_playing = false;
	m_paused = false;
	m_finished = false;

	m_videoFrameReady = false;
	m_audioFrameReady = false;

	m_currentVideoTime = 0.0;

	DEBUG_LOG("[Video playback|%s] Video loaded", m_path.string().c_str());

	return true;
}

bool VideoPlayback::openVideoDecoder()
{
	return openDecoder(m_videoStreamIndex, &m_videoCodecContext);
}

bool VideoPlayback::openAudioDecoder()
{
	return openDecoder(m_audioStreamIndex, &m_audioCodecContext);
}

bool VideoPlayback::openDecoder(int streamIndex, AVCodecContext** codecContext)
{
	if (!m_formatContext || streamIndex < 0 || !codecContext)
		return false;

	AVStream* stream = m_formatContext->streams[streamIndex];
	const AVCodecParameters* codecParameters = stream->codecpar;

	const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);

	if (!codec)
	{
		DEBUG_LOG("[Video playback|%s] Decoder not found for stream %d", m_path.string().c_str(), streamIndex);
		return false;
	}

	*codecContext = avcodec_alloc_context3(codec);

	if (!*codecContext)
	{
		DEBUG_LOG("[Video playback|%s] Could not allocate codec context", m_path.string().c_str());
		return false;
	}

	int result = avcodec_parameters_to_context(*codecContext, codecParameters);

	if (result < 0)
	{
		printFFmpegError("Could not copy codec parameters", result, m_path);
		return false;
	}

	result = avcodec_open2(*codecContext, codec, nullptr);

	if (result < 0)
	{
		printFFmpegError("Could not open decoder", result, m_path);
		return false;
	}

	return true;
}

bool VideoPlayback::play()
{
	if (!m_loaded)
		return false;

	if (m_finished)
		stop();

	m_playing = true;
	m_paused = false;

	return true;
}

void VideoPlayback::pause()
{
	if (!m_loaded)
		return;

	m_playing = false;
	m_paused = true;
}

void VideoPlayback::stop()
{
	if (!m_loaded)
		return;

	m_playing = false;
	m_paused = false;
	m_finished = false;

	m_videoFrameReady = false;
	m_audioFrameReady = false;

	m_currentVideoTime = 0.0;

	const int result = av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);

	if (result < 0)
		printFFmpegError("Could not seek to beginning of video", result, m_path);

	flushDecoders();
}

void VideoPlayback::update()
{
	if (!m_loaded || !m_playing || m_paused || m_finished)
		return;

	m_videoFrameReady = false;
	m_audioFrameReady = false;

	// For now we decode until we obtain one video frame.
	// Playback timing will be handled afterwards.
	while (!m_videoFrameReady)
	{
		const int result = av_read_frame(m_formatContext, m_packet);

		if (result < 0)
		{
			if (result != AVERROR_EOF)
				printFFmpegError("Error reading packet", result, m_path);

			m_finished = true;
			m_playing = false;

			return;
		}

		if (m_packet->stream_index == m_videoStreamIndex)
		{
			decodePacket(m_videoCodecContext, m_packet, m_videoFrame, true);
		}
		else if (m_packet->stream_index == m_audioStreamIndex)
		{
			decodePacket(m_audioCodecContext, m_packet, m_audioFrame, false);
		}
		av_packet_unref(m_packet);
	}
}

void VideoPlayback::decodePacket(AVCodecContext* codecContext, AVPacket* packet, AVFrame* frame, bool video)
{
	if (!codecContext || !packet || !frame)
		return;

	int result = avcodec_send_packet(codecContext, packet);

	if (result < 0)
	{
		if (result != AVERROR(EAGAIN) && result != AVERROR_EOF)
		{
			printFFmpegError("Error sending packet to decoder", result, m_path);
		}

		return;
	}

	while (true)
	{
		result = avcodec_receive_frame(codecContext, frame);

		if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
		{
			break;
		}

		if (result < 0)
		{
			printFFmpegError("Error receiving decoded frame", result, m_path);
			break;
		}

		if (video)
		{
			m_videoFrameReady = true;

			AVStream* stream = m_formatContext->streams[m_videoStreamIndex];

			if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
				m_currentVideoTime = frame->best_effort_timestamp * av_q2d(stream->time_base);
		}
		else
		{
			m_audioFrameReady = true;
		}

		break;
	}
}

void VideoPlayback::flushDecoders()
{
	if (m_videoCodecContext)
	{
		avcodec_flush_buffers(m_videoCodecContext);
	}

	if (m_audioCodecContext)
	{
		avcodec_flush_buffers(m_audioCodecContext);
	}
}

void VideoPlayback::unload()
{
	if (m_packet)
	{
		av_packet_free(&m_packet);
	}

	if (m_videoFrame)
	{
		av_frame_free(&m_videoFrame);
	}

	if (m_audioFrame)
	{
		av_frame_free(&m_audioFrame);
	}

	if (m_videoCodecContext)
	{
		avcodec_free_context(&m_videoCodecContext);
	}

	if (m_audioCodecContext)
	{
		avcodec_free_context(&m_audioCodecContext);
	}

	if (m_formatContext)
	{
		avformat_close_input(&m_formatContext);
	}

	m_videoStreamIndex = -1;
	m_audioStreamIndex = -1;

	m_loaded = false;
	m_playing = false;
	m_paused = false;
	m_finished = false;

	m_videoFrameReady = false;
	m_audioFrameReady = false;

	m_currentVideoTime = 0.0;
	m_duration = 0.0;

	m_path.clear();
}

bool VideoPlayback::isPlaying() const
{
	return m_playing;
}

bool VideoPlayback::isPaused() const
{
	return m_paused;
}

bool VideoPlayback::isFinished() const
{
	return m_finished;
}

const std::filesystem::path& VideoPlayback::getPath() const
{
	return m_path;
}

AVFrame* VideoPlayback::getVideoFrame() const
{
	return m_videoFrameReady ? m_videoFrame : nullptr;
}

AVFrame* VideoPlayback::getAudioFrame() const
{
	return m_audioFrameReady ? m_audioFrame : nullptr;
}

int VideoPlayback::getVideoStreamIndex() const
{
	return m_videoStreamIndex;
}

int VideoPlayback::getAudioStreamIndex() const
{
	return m_audioStreamIndex;
}

double VideoPlayback::getCurrentVideoTime() const
{
	return m_currentVideoTime;
}

double VideoPlayback::getDuration() const
{
	return m_duration;
}