#pragma once

#include "Component.h"
#include "SimpleMath.h"

class UIImage;

using DirectX::SimpleMath::Vector2;

class UISheet : public Component
{
public:
    UISheet(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void update() override;
    void drawUi() override;

    int getColumns() const { return m_columns; }
    int getRows() const { return m_rows; }
    Vector2 getOffset() const { return m_offset; }

    void setGrid(int columns, int rows);
    void setOffset(const Vector2& offset);

    void setFps(float fps) { m_fps = fps; }
    float getFps() const { return m_fps; }

    void setLoop(bool loop) { m_loop = loop; }
    bool getLoop() const { return m_loop; }

    void setPlaying(bool playing) { m_playing = playing; }
    bool isPlaying() const { return m_playing; }
    void play() { m_reverse = false; reset(); m_playing = true; }
	void playReverse() { m_reverse = true; reset(); m_playing = true; }
	void stop() { m_playing = false; }
    void reset();

    void setStartFrame(int f);
    void setEndFrame(int f);

    int getStartFrame() const { return m_startFrame; }
    int getEndFrame() const { return m_endFrame; }

    void serialize(IArchive& archive) override;

private:
    void applyToImage();
    int frameCount() const;

private:
    int m_columns = 1;
    int m_rows = 1;
    Vector2 m_offset = { 0.0f, 0.0f };

    float m_fps = 10.0f;
    bool m_loop = true;
    bool m_playing = false;
	bool m_reverse = false;

    int m_startFrame = 0;
    int m_endFrame = 0;

    float m_accum = 0.0f;
    int m_currentFrame = 0;
};
