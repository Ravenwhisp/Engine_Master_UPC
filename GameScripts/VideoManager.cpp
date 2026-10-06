#include "pch.h"
#include "VideoManager.h"
#include "UISlider.h"
#include "Transform2D.h"


IMPLEMENT_SCRIPT_FIELDS(VideoManager,
    SERIALIZED_COMPONENT_REF(m_videoObject, "Video Object", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_skipSlider, "Skip Hold Slider", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_loadingImage, "Loading Image", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_skipContainer, "Skip Container", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_sceneToLoad, "Next Scene")
)

VideoManager::VideoManager(GameObject* owner) : Script(owner)
{
}

void VideoManager::Start()
{
    if (Transform* loadingImageTransform = m_loadingImage.getReferencedComponent())
    {
        GameObject* loadingImageOwner = ComponentAPI::getOwner(loadingImageTransform);
        m_loadingImageTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(loadingImageOwner, ComponentType::TRANSFORM2D));
    }

    if (Transform* skipContainerTransform = m_skipContainer.getReferencedComponent())
    {
        m_skipContainerOwner = ComponentAPI::getOwner(skipContainerTransform);
        m_skipContainerTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(m_skipContainerOwner, ComponentType::TRANSFORM2D));
        GameObjectAPI::setActive(m_skipContainerOwner, false);
    }

    if (Transform* sliderTransform = m_skipSlider.getReferencedComponent())
    {
        GameObject* sliderOwner = ComponentAPI::getOwner(sliderTransform);
        m_skipSliderComponent = static_cast<UISlider*>(GameObjectAPI::getComponent(sliderOwner, ComponentType::UISLIDER));
    }

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

    SceneAPI::beginAsyncSceneLoad(m_sceneToLoad.c_str());

}

void VideoManager::Update()
{
    if (!m_videoComponent)
    {
        return;
    }

    if (Input::isFaceButtonBottomPressed(0) && SceneAPI::isAsyncSceneLoadReady())
    {
        m_gamepadSkipHoldTime += Time::getDeltaTime();
    }
    else
    {
        m_gamepadSkipHoldTime = 0.0f;
    }

    if (m_skipSliderComponent)
    {
        const float holdProgress = m_gamepadSkipHoldTime >= 3.0f ? 1.0f : m_gamepadSkipHoldTime / 3.0f;
        SliderAPI::setFillAmount(m_skipSliderComponent, holdProgress);
    }

    if (SceneAPI::isAsyncSceneLoadReady()) {
        GameObjectAPI::setActive(m_skipContainerOwner, true);
    }

    const bool skipRequested = SceneAPI::isAsyncSceneLoadReady() && (Input::isKeyDown(KeyCode::Escape) || m_gamepadSkipHoldTime >= 3.0f);
    const bool finished = m_started && !VideoAPI::isPlaying(m_videoComponent);

    if (skipRequested || finished)
    {
        VideoAPI::stop(m_videoComponent);

        if (m_loadingImageTransform)
        {
            Transform2DAPI::setAlpha(m_loadingImageTransform, 1.0f);
        }

        if (m_skipContainerTransform)
        {
            Transform2DAPI::setAlpha(m_skipContainerTransform, 0.0f);
        }

        if (!m_sceneToLoad.empty())
        {
            SceneAPI::requestAsyncSceneChange();
        }
    }
}

IMPLEMENT_SCRIPT(VideoManager)
