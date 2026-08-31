#pragma once

#include <deque>
#include <filesystem>
#include <vector>

#include <xaudio2.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}

class VideoPlayback
{
private:
	struct VideoFrame
	{
		AVFrame* frame = nullptr;
		double timestamp = 0.0;
	};

	class AudioCallback final : public IXAudio2VoiceCallback
	{
	public:
		void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
		void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
		void STDMETHODCALLTYPE OnStreamEnd() override {}
		void STDMETHODCALLTYPE OnBufferStart(void*) override {}

		void STDMETHODCALLTYPE OnBufferEnd(void* context) override
		{
			delete static_cast<std::vector<uint8_t>*>(context);
		}

		void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
		void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}
	};

private:
	static constexpr UINT32 MAX_AUDIO_BUFFERS = 4;
	static constexpr size_t MAX_VIDEO_FRAMES = 8;

	std::filesystem::path m_path;

	std::string m_displayName;

	std::vector<uint8_t> m_buffer;
	AVIOContext* m_avioContext = nullptr;
	void* m_avioOpaque = nullptr;

	AVFormatContext* m_formatContext = nullptr;

	AVCodecContext* m_videoCodecContext = nullptr;
	AVCodecContext* m_audioCodecContext = nullptr;

	AVPacket* m_packet = nullptr;
	std::deque<std::unique_ptr<AVPacket>> m_pendingAudioPackets;
	std::deque<std::unique_ptr<AVPacket>> m_pendingVideoPackets;

	AVFrame* m_decodeVideoFrame = nullptr;
	AVFrame* m_videoFrame = nullptr;
	AVFrame* m_audioFrame = nullptr;

	std::deque<VideoFrame> m_videoFrames;

	SwrContext* m_swrContext = nullptr;

	IXAudio2* m_xAudio = nullptr;
	IXAudio2SourceVoice* m_sourceVoice = nullptr;

	AudioCallback m_audioCallback;

	int m_videoStreamIndex = -1;
	int m_audioStreamIndex = -1;

	bool m_loaded = false;
	bool m_playing = false;
	bool m_paused = false;
	bool m_finished = false;
	bool m_endOfFile = false;

	bool m_hasVideoFrame = false;
	bool m_videoFrameReady = false;

	double m_playbackTime = 0.0;
	double m_currentVideoTime = 0.0;
	double m_duration = 0.0;

public:
	explicit VideoPlayback(IXAudio2* xAudio);
	~VideoPlayback();

	VideoPlayback(const VideoPlayback&) = delete;
	VideoPlayback& operator=(const VideoPlayback&) = delete;

	bool load(const std::filesystem::path& path);
	bool loadFromBuffer(const uint8_t* data, size_t size);
	void unload();

	bool play();
	void pause();
	void stop();

	void update();

	bool isPlaying() const;
	bool isPaused() const;
	bool isFinished() const;

	const std::filesystem::path& getPath() const;
	void setName(const std::string& name) { m_displayName = name; }
	const std::string& getName() const { return m_displayName; }

	AVFrame* getVideoFrame() const;

	int getVideoStreamIndex() const;
	int getAudioStreamIndex() const;

	double getPlaybackTime() const;
	double getCurrentVideoTime() const;
	double getDuration() const;

	bool hasNewVideoFrame() const { return m_videoFrameReady; }

private:
	bool openVideoDecoder();
	bool openAudioDecoder();
	bool openDecoder(int streamIndex, AVCodecContext** codecContext);

	bool finalizeLoad();

	bool initAudio();
	void cleanUpAudio();

	bool decodeVideoPacket(AVPacket* packet);
	bool decodeAudioPacket(AVPacket* packet);

	bool receiveVideoFrames();
	bool receiveAudioFrames();

	bool queueAudioFrame(AVFrame* frame);
	bool canQueueAudio() const;

	void fillDecodeQueues();
	void updateVideoFrame();

	void clearVideoFrames();
	void clearPendingPackets();

	void flushDecoders();
	void flushDelayedFrames();
};
