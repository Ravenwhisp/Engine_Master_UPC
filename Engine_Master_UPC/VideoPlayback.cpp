#include "Globals.h"
#include "VideoPlayback.h"

#include "Application.h"
#include "ModuleTime.h"

#include <cstdio>
#include <cstring>

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

	struct AvioBuffer
	{
		const uint8_t* data = nullptr;
		size_t size = 0;
		size_t offset = 0;
	};

	int avioRead(void* opaque, uint8_t* buf, int bufSize)
	{
		auto* ctx = static_cast<AvioBuffer*>(opaque);

		if (!ctx || ctx->offset >= ctx->size)
			return AVERROR_EOF;

		int toRead = bufSize;
		if (ctx->offset + static_cast<size_t>(toRead) > ctx->size)
			toRead = static_cast<int>(ctx->size - ctx->offset);

		std::memcpy(buf, ctx->data + ctx->offset, static_cast<size_t>(toRead));
		ctx->offset += static_cast<size_t>(toRead);

		return toRead;
	}

	int64_t avioSeek(void* opaque, int64_t offset, int whence)
	{
		auto* ctx = static_cast<AvioBuffer*>(opaque);
		if (!ctx)
			return -1;

		if ((whence & ~AVSEEK_FORCE) == AVSEEK_SIZE)
			return static_cast<int64_t>(ctx->size);

		const int seekMode = whence & ~AVSEEK_FORCE;
		int64_t newOffset = 0;

		switch (seekMode)
		{
		case SEEK_SET:
			newOffset = offset;
			break;
		case SEEK_CUR:
			newOffset = static_cast<int64_t>(ctx->offset) + offset;
			break;
		case SEEK_END:
			newOffset = static_cast<int64_t>(ctx->size) + offset;
			break;
		default:
			return -1;
		}

		if (newOffset < 0 || newOffset > static_cast<int64_t>(ctx->size))
			return -1;

		ctx->offset = static_cast<size_t>(newOffset);
		return newOffset;
	}
}

VideoPlayback::VideoPlayback(IXAudio2* xAudio) : m_xAudio(xAudio)
{
}

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

	return finalizeLoad();
}

bool VideoPlayback::loadFromBuffer(const uint8_t* data, size_t size)
{
	unload();

	if (!data || size == 0)
		return false;

	m_buffer.assign(data, data + size);
	m_avioOpaque = new AvioBuffer{ m_buffer.data(), m_buffer.size(), 0 };

	constexpr int kBufferSize = 4096;
	uint8_t* avioBuffer = static_cast<uint8_t*>(av_malloc(kBufferSize));
	if (!avioBuffer)
	{
		delete static_cast<AvioBuffer*>(m_avioOpaque);
		m_avioOpaque = nullptr;
		return false;
	}

	m_avioContext = avio_alloc_context(avioBuffer, kBufferSize, 0, m_avioOpaque, &avioRead, nullptr, &avioSeek);
	if (!m_avioContext)
	{
		av_free(avioBuffer);
		delete static_cast<AvioBuffer*>(m_avioOpaque);
		m_avioOpaque = nullptr;
		return false;
	}
	m_avioContext->seekable = AVIO_SEEKABLE_NORMAL;

	m_formatContext = avformat_alloc_context();
	if (!m_formatContext)
	{
		unload();
		return false;
	}

	m_formatContext->pb = m_avioContext;
	m_formatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

	int result = avformat_open_input(&m_formatContext, nullptr, nullptr, nullptr);

	if (result < 0)
	{
		printFFmpegError("Could not open video", result, m_path);
		unload();
		return false;
	}

	return finalizeLoad();
}

bool VideoPlayback::finalizeLoad()
{
	int result = avformat_find_stream_info(m_formatContext, nullptr);

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
		DEBUG_ERROR("[Video playback|%s] No video stream found", m_path.string().c_str());
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

	m_decodeVideoFrame = av_frame_alloc();
	m_videoFrame = av_frame_alloc();

	if (m_audioStreamIndex >= 0)
		m_audioFrame = av_frame_alloc();

	if (!m_packet || !m_decodeVideoFrame || !m_videoFrame || (m_audioStreamIndex >= 0 && !m_audioFrame))
	{
		DEBUG_ERROR("[Video playback|%s] Could not allocate FFmpeg packet/frame", m_path.string().c_str());
		unload();
		return false;
	}

	if (m_audioStreamIndex >= 0 && !initAudio())
	{
		unload();
		return false;
	}

	if (m_formatContext->duration != AV_NOPTS_VALUE)
		m_duration = static_cast<double>(m_formatContext->duration) / static_cast<double>(AV_TIME_BASE);

	m_loaded = true;
	m_playing = false;
	m_paused = false;
	m_finished = false;
	m_endOfFile = false;

	m_videoFrameReady = false;
	m_hasVideoFrame = false;

	m_playbackTime = 0.0;
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
		DEBUG_ERROR("[Video playback|%s] Decoder not found for stream %d", m_path.string().c_str(), streamIndex);
		return false;
	}

	*codecContext = avcodec_alloc_context3(codec);

	if (!*codecContext)
		return false;

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

bool VideoPlayback::initAudio()
{
	if (!m_xAudio || !m_audioCodecContext)
		return false;

	WAVEFORMATEX format = {};
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = m_audioCodecContext->sample_rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	HRESULT result = m_xAudio->CreateSourceVoice(&m_sourceVoice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &m_audioCallback);

	if (FAILED(result))
	{
		DEBUG_ERROR("[Video playback|%s] Could not create XAudio2 source voice", m_path.string().c_str());
		return false;
	}

	AVChannelLayout outputLayout = AV_CHANNEL_LAYOUT_STEREO;

	int swrResult = swr_alloc_set_opts2(&m_swrContext, &outputLayout, AV_SAMPLE_FMT_S16, m_audioCodecContext->sample_rate,
		&m_audioCodecContext->ch_layout, m_audioCodecContext->sample_fmt, m_audioCodecContext->sample_rate, 0, nullptr);

	if (swrResult < 0)
	{
		printFFmpegError("Could not create audio resampler", swrResult, m_path);
		return false;
	}

	swrResult = swr_init(m_swrContext);

	if (swrResult < 0)
	{
		printFFmpegError("Could not initialize audio resampler", swrResult, m_path);
		return false;
	}

	DEBUG_LOG("[Video playback|%s] Audio: %d Hz | input channels: %d | output channels: 2 | input format: %d",
		m_path.string().c_str(),
		m_audioCodecContext->sample_rate,
		m_audioCodecContext->ch_layout.nb_channels,
		m_audioCodecContext->sample_fmt);

	return true;
}

void VideoPlayback::cleanUpAudio()
{
	if (m_sourceVoice)
	{
		m_sourceVoice->Stop();
		m_sourceVoice->FlushSourceBuffers();
		m_sourceVoice->DestroyVoice();
		m_sourceVoice = nullptr;
	}

	if (m_swrContext)
		swr_free(&m_swrContext);
}

bool VideoPlayback::play()
{
	if (!m_loaded)
		return false;

	if (m_finished)
		stop();

	fillDecodeQueues();

	if (m_sourceVoice)
		m_sourceVoice->Start();

	m_playing = true;
	m_paused = false;

	return true;
}

void VideoPlayback::pause()
{
	if (!m_loaded)
		return;

	if (m_sourceVoice)
		m_sourceVoice->Stop();

	m_playing = false;
	m_paused = true;
}

void VideoPlayback::stop()
{
	if (!m_loaded)
		return;

	if (m_sourceVoice)
	{
		m_sourceVoice->Stop();
		m_sourceVoice->FlushSourceBuffers();
	}

	m_playing = false;
	m_paused = false;
	m_finished = false;
	m_endOfFile = false;

	m_videoFrameReady = false;
	m_hasVideoFrame = false;

	m_playbackTime = 0.0;
	m_currentVideoTime = 0.0;

	clearVideoFrames();
	clearPendingPackets();

	if (m_videoFrame)
		av_frame_unref(m_videoFrame);

	if (m_decodeVideoFrame)
		av_frame_unref(m_decodeVideoFrame);

	if (m_audioFrame)
		av_frame_unref(m_audioFrame);

	const int result = av_seek_frame(m_formatContext, -1, 0, AVSEEK_FLAG_BACKWARD);

	if (result < 0)
		printFFmpegError("Could not seek to beginning of video", result, m_path);

	flushDecoders();
}

void VideoPlayback::update()
{
	if (!m_loaded || !m_playing || m_paused || m_finished)
		return;

	m_playbackTime += app->getModuleTime()->deltaTime();

	fillDecodeQueues();
	updateVideoFrame();

	if (!m_endOfFile)
		return;

	XAUDIO2_VOICE_STATE state = {};

	if (m_sourceVoice)
		m_sourceVoice->GetState(&state);

	if (m_videoFrames.empty() && (!m_sourceVoice || state.BuffersQueued == 0))
	{
		m_finished = true;
		m_playing = false;
	}
}

void VideoPlayback::fillDecodeQueues()
{
	if (m_endOfFile && m_pendingAudioPackets.empty() && m_pendingVideoPackets.empty())
		return;

	// Audio and video have independent queues. Previously one blocked audio packet stopped the
	// demuxer, which also stopped video decoding and made current video time wait for XAudio.
	while (true)
	{
		AVPacket* packet = nullptr;
		std::deque<std::unique_ptr<AVPacket>>* pendingQueue = nullptr;
		const bool videoHasRoom = m_videoFrames.size() < MAX_VIDEO_FRAMES;
		const bool audioHasRoom = canQueueAudio();

		// Consume whichever pending stream can currently make progress. Video is preferred so its
		// presentation queue remains ahead of the playback clock.
		if (videoHasRoom && !m_pendingVideoPackets.empty())
		{
			pendingQueue = &m_pendingVideoPackets;
			packet = pendingQueue->front().get();
		}
		else if (audioHasRoom && !m_pendingAudioPackets.empty())
		{
			pendingQueue = &m_pendingAudioPackets;
			packet = pendingQueue->front().get();
		}
		else
		{
			if (!videoHasRoom && !audioHasRoom)
				break;

			if (m_endOfFile)
			{
				flushDelayedFrames();
				break;
			}

			const int result = av_read_frame(m_formatContext, m_packet);

			if (result < 0)
			{
				if (result == AVERROR_EOF)
					m_endOfFile = true;
				else
				{
					printFFmpegError("Error reading packet", result, m_path);
					m_finished = true;
					m_playing = false;
				}

				if (m_endOfFile)
				{
					flushDelayedFrames();
					if (m_pendingAudioPackets.empty() && m_pendingVideoPackets.empty())
						break;
					continue;
				}

				break;
			}

			packet = m_packet;

			// Preserve blocked packets instead of stopping the other stream. Each stream has its own
			// pending queue, so an audio backlog can never prevent video from being decoded.
			if (packet->stream_index == m_videoStreamIndex && !videoHasRoom)
			{
				auto pending = std::make_unique<AVPacket>();
				const int refResult = av_packet_ref(pending.get(), packet);
				if (refResult < 0)
				{
					printFFmpegError("Could not store pending video packet", refResult, m_path);
					av_packet_unref(m_packet);
					return;
				}
				m_pendingVideoPackets.push_back(std::move(pending));
				av_packet_unref(m_packet);
				continue;
			}

			if (packet->stream_index == m_audioStreamIndex && !audioHasRoom)
			{
				auto pending = std::make_unique<AVPacket>();
				const int refResult = av_packet_ref(pending.get(), packet);
				if (refResult < 0)
				{
					printFFmpegError("Could not store pending audio packet", refResult, m_path);
					av_packet_unref(m_packet);
					return;
				}
				m_pendingAudioPackets.push_back(std::move(pending));
				av_packet_unref(m_packet);
				continue;
			}
		}

		bool decoded = true;

		if (packet->stream_index == m_videoStreamIndex)
			decoded = decodeVideoPacket(packet);
		else if (packet->stream_index == m_audioStreamIndex)
			decoded = decodeAudioPacket(packet);

		if (pendingQueue)
		{
			av_packet_unref(pendingQueue->front().get());
			pendingQueue->pop_front();
		}
		else
			av_packet_unref(m_packet);

		if (!decoded)
		{
			m_finished = true;
			m_playing = false;
			return;
		}

	}
}

bool VideoPlayback::decodeVideoPacket(AVPacket* packet)
{
	if (!packet || !m_videoCodecContext)
		return false;

	int result = avcodec_send_packet(m_videoCodecContext, packet);

	if (result == AVERROR(EAGAIN))
	{
		if (!receiveVideoFrames())
			return false;

		result = avcodec_send_packet(m_videoCodecContext, packet);
	}

	if (result < 0 && result != AVERROR_EOF)
	{
		printFFmpegError("Could not send video packet", result, m_path);
		return false;
	}

	return receiveVideoFrames();
}

bool VideoPlayback::decodeAudioPacket(AVPacket* packet)
{
	if (!packet || !m_audioCodecContext)
		return false;

	int result = avcodec_send_packet(m_audioCodecContext, packet);

	if (result == AVERROR(EAGAIN))
	{
		if (!receiveAudioFrames())
			return false;

		result = avcodec_send_packet(m_audioCodecContext, packet);
	}

	if (result < 0 && result != AVERROR_EOF)
	{
		printFFmpegError("Could not send audio packet", result, m_path);
		return false;
	}

	return receiveAudioFrames();
}

bool VideoPlayback::receiveVideoFrames()
{
	// Drain the decoder completely: pull every frame it has available, not just until our
	// display queue is "full". Stopping early leaves buffered frames inside the decoder, which
	// makes the next avcodec_send_packet() return AVERROR(EAGAIN) ("Resource temporarily
	// unavailable"). The display queue may temporarily exceed MAX_VIDEO_FRAMES by a few frames;
	// updateVideoFrame() consumes them over time, so this is safe.
	while (true)
	{
		const int result = avcodec_receive_frame(m_videoCodecContext, m_decodeVideoFrame);

		if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
			return true;

		if (result < 0)
		{
			printFFmpegError("Could not receive video frame", result, m_path);
			return false;
		}

		AVFrame* frame = av_frame_clone(m_decodeVideoFrame);

		if (!frame)
			return false;

		double timestamp = 0.0;

		if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
		{
			AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
			timestamp = frame->best_effort_timestamp * av_q2d(stream->time_base);
		}

		m_videoFrames.push_back({ frame, timestamp });

		av_frame_unref(m_decodeVideoFrame);
	}

	return true;
}

bool VideoPlayback::receiveAudioFrames()
{
	// Same reasoning as receiveVideoFrames: fully drain the decoder so a later avcodec_send_packet()
	// never hits AVERROR(EAGAIN). We only *submit* to XAudio while there is room (canQueueAudio),
	// but we keep pulling from the decoder until it has no more output for this packet.
	while (true)
	{
		if (!canQueueAudio())
			break;
		const int result = avcodec_receive_frame(m_audioCodecContext, m_audioFrame);

		if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
			return true;

		if (result < 0)
		{
			printFFmpegError("Could not receive audio frame", result, m_path);
			return false;
		}

		if (!queueAudioFrame(m_audioFrame))
			return false;

		av_frame_unref(m_audioFrame);
	}

	return true;
}

void VideoPlayback::updateVideoFrame()
{
	m_videoFrameReady = false;

	while (!m_videoFrames.empty() && m_videoFrames.front().timestamp <= m_playbackTime)
	{
		VideoFrame& queuedFrame = m_videoFrames.front();

		av_frame_unref(m_videoFrame);

		const int result = av_frame_ref(m_videoFrame, queuedFrame.frame);

		if (result < 0)
		{
			printFFmpegError("Could not reference video frame", result, m_path);
			return;
		}

		m_currentVideoTime = queuedFrame.timestamp;
		m_videoFrameReady = true;
		m_hasVideoFrame = true;

		av_frame_free(&queuedFrame.frame);
		m_videoFrames.pop_front();
	}
}

bool VideoPlayback::canQueueAudio() const
{
	if (!m_sourceVoice)
		return false;

	XAUDIO2_VOICE_STATE state = {};
	m_sourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	return state.BuffersQueued < MAX_AUDIO_BUFFERS;
}

bool VideoPlayback::queueAudioFrame(AVFrame* frame)
{
	if (!frame || !m_swrContext || !m_sourceVoice)
		return false;

	const int outputSamples = swr_get_out_samples(m_swrContext, frame->nb_samples);

	if (outputSamples <= 0)
		return false;

	const int outputChannels = 2;
	const int bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

	auto* audioData = new std::vector<uint8_t>();
	audioData->resize(static_cast<size_t>(outputSamples) * outputChannels * bytesPerSample);

	uint8_t* outputBuffer = audioData->data();

	const int convertedSamples = swr_convert(m_swrContext, &outputBuffer, outputSamples,
		const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);

	if (convertedSamples < 0)
	{
		printFFmpegError("Could not convert audio samples", convertedSamples, m_path);
		delete audioData;
		return false;
	}

	audioData->resize(static_cast<size_t>(convertedSamples) * outputChannels * bytesPerSample);

	if (audioData->empty())
	{
		delete audioData;
		return true;
	}

	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(audioData->size());
	buffer.pAudioData = audioData->data();
	buffer.pContext = audioData;

	const HRESULT result = m_sourceVoice->SubmitSourceBuffer(&buffer);

	if (FAILED(result))
	{
		DEBUG_ERROR("[Video playback|%s] Could not submit XAudio2 buffer", m_path.string().c_str());
		delete audioData;
		return false;
	}

	return true;
}

void VideoPlayback::clearVideoFrames()
{
	for (VideoFrame& videoFrame : m_videoFrames)
	{
		if (videoFrame.frame)
			av_frame_free(&videoFrame.frame);
	}

	m_videoFrames.clear();
}

void VideoPlayback::clearPendingPackets()
{
	for (auto& packet : m_pendingAudioPackets)
		av_packet_unref(packet.get());
	m_pendingAudioPackets.clear();

	for (auto& packet : m_pendingVideoPackets)
		av_packet_unref(packet.get());
	m_pendingVideoPackets.clear();
}

void VideoPlayback::flushDecoders()
{
	if (m_videoCodecContext)
		avcodec_flush_buffers(m_videoCodecContext);

	if (m_audioCodecContext)
		avcodec_flush_buffers(m_audioCodecContext);

	if (m_swrContext)
	{
		swr_close(m_swrContext);

		const int result = swr_init(m_swrContext);

		if (result < 0)
			printFFmpegError("Could not reset audio resampler", result, m_path);
	}
}

void VideoPlayback::flushDelayedFrames()
{
	if (m_videoCodecContext)
	{
		const int result = avcodec_send_packet(m_videoCodecContext, nullptr);

		if (result >= 0 || result == AVERROR_EOF)
			receiveVideoFrames();
	}

	if (m_audioCodecContext && canQueueAudio())
	{
		const int result = avcodec_send_packet(m_audioCodecContext, nullptr);

		if (result >= 0 || result == AVERROR_EOF)
			receiveAudioFrames();
	}
}

void VideoPlayback::unload()
{
	cleanUpAudio();

	clearVideoFrames();
	clearPendingPackets();

	if (m_packet)
		av_packet_free(&m_packet);

	if (m_decodeVideoFrame)
		av_frame_free(&m_decodeVideoFrame);

	if (m_videoFrame)
		av_frame_free(&m_videoFrame);

	if (m_audioFrame)
		av_frame_free(&m_audioFrame);

	if (m_videoCodecContext)
		avcodec_free_context(&m_videoCodecContext);

	if (m_audioCodecContext)
		avcodec_free_context(&m_audioCodecContext);

	if (m_formatContext)
		avformat_close_input(&m_formatContext);

	if (m_avioContext)
	{
		av_free(m_avioContext->buffer);
		avio_context_free(&m_avioContext);
		m_avioContext = nullptr;
	}

	if (m_avioOpaque)
	{
		delete static_cast<AvioBuffer*>(m_avioOpaque);
		m_avioOpaque = nullptr;
	}

	m_buffer.clear();
	m_displayName.clear();

	m_videoStreamIndex = -1;
	m_audioStreamIndex = -1;

	m_loaded = false;
	m_playing = false;
	m_paused = false;
	m_finished = false;
	m_endOfFile = false;

	m_videoFrameReady = false;
	m_hasVideoFrame = false;

	m_playbackTime = 0.0;
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
	return m_hasVideoFrame ? m_videoFrame : nullptr;
}

int VideoPlayback::getVideoStreamIndex() const
{
	return m_videoStreamIndex;
}

int VideoPlayback::getAudioStreamIndex() const
{
	return m_audioStreamIndex;
}

double VideoPlayback::getPlaybackTime() const
{
	return m_playbackTime;
}

double VideoPlayback::getCurrentVideoTime() const
{
	return m_currentVideoTime;
}

double VideoPlayback::getDuration() const
{
	return m_duration;
}
