#include "pch.h"
#include "VideoManager.h"


IMPLEMENT_SCRIPT_FIELDS(VideoManager,
    SERIALIZED_COMPONENT_REF(m_videoObject, "Video Object", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_sceneToLoad, "Next Scene")
)

VideoManager::VideoManager(GameObject* owner)
    : Script(owner)
{
}

void VideoManager::Start()
{
    GameObject* videoOwner = getOwner();
    if (Transform* videoObjectTransform = m_videoObject.getReferencedComponent())
    {
        videoOwner = ComponentAPI::getOwner(videoObjectTransform);
    }

    m_videoComponent = VideoAPI::getVideoComponent(videoOwner);

    if (m_videoComponent)
    {
        VideoAPI::play(m_videoComponent);
        m_started = true;
    }
}

void VideoManager::Update()
{
    if (!m_videoComponent)
    {
        return;
    }

    const bool skipRequested = Input::isKeyDown(KeyCode::Escape);
    const bool finished = m_started && !VideoAPI::isPlaying(m_videoComponent);

    if (skipRequested || finished)
    {
        VideoAPI::stop(m_videoComponent);

        if (!m_sceneToLoad.empty())
        {
            SceneAPI::requestSceneChange(m_sceneToLoad.c_str());
        }
    }
}

IMPLEMENT_SCRIPT(VideoManager)
