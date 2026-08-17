#pragma once

#include <filesystem>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
}

class VideoPlayback
{
private:
	std::filesystem::path m_path;

	AVFormatContext* m_formatContext = nullptr;

	AVCodecContext* m_videoCodecContext = nullptr;
	AVCodecContext* m_audioCodecContext = nullptr;

	AVPacket* m_packet = nullptr;

	AVFrame* m_videoFrame = nullptr;
	AVFrame* m_audioFrame = nullptr;

	int m_videoStreamIndex = -1;
	int m_audioStreamIndex = -1;

	bool m_loaded = false;
	bool m_playing = false;
	bool m_paused = false;
	bool m_finished = false;

	bool m_videoFrameReady = false;
	bool m_audioFrameReady = false;

	double m_currentVideoTime = 0.0;
	double m_duration = 0.0;

public:
	VideoPlayback();
	~VideoPlayback();

	VideoPlayback(const VideoPlayback&) = delete;
	VideoPlayback& operator=(const VideoPlayback&) = delete;

	bool load(const std::filesystem::path& path);
	void unload();

	bool play();
	void pause();
	void stop();

	void update();

	bool isPlaying() const;
	bool isPaused() const;
	bool isFinished() const;

	const std::filesystem::path& getPath() const;

	AVFrame* getVideoFrame() const;
	AVFrame* getAudioFrame() const;

	int getVideoStreamIndex() const;
	int getAudioStreamIndex() const;

	double getCurrentVideoTime() const;
	double getDuration() const;

private:
	bool openVideoDecoder();
	bool openAudioDecoder();
	bool openDecoder(int streamIndex, AVCodecContext** codecContext);

	void decodePacket(AVCodecContext* codecContext, AVPacket* packet, AVFrame* frame, bool video);
	void flushDecoders();

};