#include "Globals.h"
#include "UISheet.h"

#include <imgui.h>

#include "Application.h"
#include "ModuleTime.h"

#include "GameObject.h"
#include "UIImage.h"

UISheet::UISheet(UID id, GameObject* owner)
    : Component(id, ComponentType::UISHEET, owner)
{
    m_endFrame = frameCount() - 1;
    m_currentFrame = m_startFrame;
    applyToImage();
}

std::unique_ptr<Component> UISheet::clone(GameObject* newOwner) const
{
    std::unique_ptr<UISheet> cloned = std::make_unique<UISheet>(m_uuid, newOwner);

    cloned->setActive(this->isActive());

    cloned->m_columns = m_columns;
    cloned->m_rows = m_rows;

    cloned->m_fps = m_fps;
    cloned->m_loop = m_loop;
    cloned->m_playing = m_playing;

    cloned->m_startFrame = m_startFrame;
    cloned->m_endFrame = m_endFrame;

    cloned->m_accum = m_accum;
    cloned->m_currentFrame = m_currentFrame;

    cloned->applyToImage();

    return cloned;
}

void UISheet::setGrid(int columns, int rows)
{
    m_columns = std::max(1, columns);
    m_rows = std::max(1, rows);

    m_endFrame = std::clamp(m_endFrame, 0, frameCount() - 1);
    m_startFrame = std::clamp(m_startFrame, 0, m_endFrame);
    m_currentFrame = std::clamp(m_currentFrame, m_startFrame, m_endFrame);

    applyToImage();
}

void UISheet::reset()
{
    m_accum = 0.0f;
    if (m_reverse)
    {
        m_currentFrame = m_endFrame;
	}
    else
    {
        m_currentFrame = m_startFrame;
	}
    applyToImage();
}

void UISheet::setStartFrame(int f)
{
    m_startFrame = std::max(0, f);
    m_endFrame = std::max(m_endFrame, m_startFrame);
    m_endFrame = std::min(m_endFrame, frameCount() - 1);
    m_currentFrame = std::clamp(m_currentFrame, m_startFrame, m_endFrame);
    applyToImage();
}

void UISheet::setEndFrame(int f)
{
    m_endFrame = std::max(0, f);
    m_endFrame = std::min(m_endFrame, frameCount() - 1);
    m_startFrame = std::min(m_startFrame, m_endFrame);
    m_currentFrame = std::clamp(m_currentFrame, m_startFrame, m_endFrame);
    applyToImage();
}

int UISheet::frameCount() const
{
    return std::max(1, m_columns * m_rows);
}

void UISheet::applyToImage()
{
    if (!getOwner())
    {
        return;
    }

    UIImage* img = getOwner()->GetComponentAs<UIImage>(ComponentType::UIIMAGE);
    if (!img)
    {
        return;
    }

    img->setSheetGrid(m_columns, m_rows);

    const int index = std::clamp(m_currentFrame, 0, frameCount() - 1);
    const int col = (m_columns > 0) ? (index % m_columns) : 0;
    const int row = (m_columns > 0) ? (index / m_columns) : 0;

    const float invCols = 1.0f / float(std::max(1, m_columns));
    const float invRows = 1.0f / float(std::max(1, m_rows));

    const Vector2 uvOffset = { float(col) * invCols, + float(row) * invRows };
    img->setSheetOffset(uvOffset);
}

void UISheet::update()
{
    if (!isActive() || !getOwner() || !getOwner()->GetActive())
    {
        return;
    }

    if (!m_playing)
    {
        return;
    }

    const float fps = std::max(0.0f, m_fps);
    if (fps <= 0.0f)
    {
        return;
    }

    const float dt = app->getModuleTime()->deltaTime();
    m_accum += dt;

    const float frameTime = 1.0f / fps;
    while (m_accum >= frameTime)
    {
        m_accum -= frameTime;

        if (m_reverse && m_currentFrame > m_startFrame)
        {
            --m_currentFrame;
        }
        else if (!m_reverse && m_currentFrame < m_endFrame)
        {
            ++m_currentFrame;
        }
        else
        {
            if (m_loop)
            {
                if (m_reverse)
                {
                    m_currentFrame = m_endFrame;
                }
				else
                {
                    m_currentFrame = m_startFrame;
                }
            }
            else
            {
                m_playing = false;
                applyToImage();
                break;
            }
        }
    }

    applyToImage();
}

void UISheet::drawUi()
{
    ImGui::Text("UISheet");

    int cols = m_columns;
    int rows = m_rows;
    if (ImGui::DragInt("Columns", &cols, 1.0f, 1, 1024) || ImGui::DragInt("Rows", &rows, 1.0f, 1, 1024))
    {
        setGrid(cols, rows);
    }

    ImGui::Separator();

	if (m_playing)
    {
        if (ImGui::Button("Stop"))
        {
            stop();
        }
        ImGui::SameLine();
		ImGui::Text("Playing %s", m_reverse ? "Reverse" : "Forward");
    }
    else
    {
        if (ImGui::Button("Play"))
        {
            play();
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Reverse"))
        {
            playReverse();
        }
	}
    ImGui::Checkbox("Loop", &m_loop);
    ImGui::DragFloat("FPS", &m_fps, 0.1f, 0.0f, 240.0f);

    const int totalFrames = frameCount();
    ImGui::Text("Frames: %d", totalFrames);

    int start = m_startFrame;
    int end = m_endFrame;
    int cur = m_currentFrame;

    if (ImGui::DragInt("Start Frame", &start, 1.0f, 0, totalFrames - 1))
    {
        setStartFrame(start);
    }
    if (ImGui::DragInt("End Frame", &end, 1.0f, 0, totalFrames - 1))
    {
        setEndFrame(end);
    }
    if (ImGui::SliderInt("Current Frame", &cur, m_startFrame, m_endFrame))
    {
        m_currentFrame = cur;
        applyToImage();
    }

    applyToImage();
}

rapidjson::Value UISheet::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::UISHEET), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("Columns", m_columns, domTree.GetAllocator());
    componentInfo.AddMember("Rows", m_rows, domTree.GetAllocator());

    componentInfo.AddMember("FPS", m_fps, domTree.GetAllocator());
    componentInfo.AddMember("Loop", m_loop, domTree.GetAllocator());
    componentInfo.AddMember("Playing", m_playing, domTree.GetAllocator());

    componentInfo.AddMember("StartFrame", m_startFrame, domTree.GetAllocator());
    componentInfo.AddMember("EndFrame", m_endFrame, domTree.GetAllocator());
    componentInfo.AddMember("CurrentFrame", m_currentFrame, domTree.GetAllocator());

    return componentInfo;
}

bool UISheet::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("Columns")) m_columns = std::max(1, componentInfo["Columns"].GetInt());
    if (componentInfo.HasMember("Rows")) m_rows = std::max(1, componentInfo["Rows"].GetInt());

    if (componentInfo.HasMember("FPS")) m_fps = componentInfo["FPS"].GetFloat();
    if (componentInfo.HasMember("Loop")) m_loop = componentInfo["Loop"].GetBool();
    if (componentInfo.HasMember("Playing")) m_playing = componentInfo["Playing"].GetBool();

    m_startFrame = componentInfo.HasMember("StartFrame") ? componentInfo["StartFrame"].GetInt() : 0;
    m_endFrame = componentInfo.HasMember("EndFrame") ? componentInfo["EndFrame"].GetInt() : (frameCount() - 1);
    m_currentFrame = componentInfo.HasMember("CurrentFrame") ? componentInfo["CurrentFrame"].GetInt() : m_startFrame;

    m_endFrame = std::clamp(m_endFrame, 0, frameCount() - 1);
    m_startFrame = std::clamp(m_startFrame, 0, m_endFrame);
    m_currentFrame = std::clamp(m_currentFrame, m_startFrame, m_endFrame);

    applyToImage();

    return true;
}
