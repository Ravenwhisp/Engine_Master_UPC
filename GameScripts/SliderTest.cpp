#include "pch.h"
#include "SliderTest.h"

IMPLEMENT_SCRIPT_FIELDS(SliderTest,
    SERIALIZED_COMPONENT_REF(m_slider, "Slider", ComponentType::UISLIDER),
    SERIALIZED_FLOAT(m_fillSpeed, "Fill Speed", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_stepAmount, "Step Amount", 0.01f, 1.0f, 0.01f),
    SERIALIZED_BOOL(m_autoTest, "Auto Test")
)

SliderTest::SliderTest(GameObject* owner)
    : Script(owner)
{
}


void SliderTest::Start()
{
    UISlider* slider = m_slider.getReferencedComponent();
    if (!slider)
    {
        Debug::warn("[UISliderTest] No slider assigned.");
        return;
    }

    m_currentFill = SliderAPI::getFillAmount(slider);
    Debug::log("[UISliderTest] Slider found. Initial fill = %.2f", m_currentFill);
}

void SliderTest::Update()
{
    UISlider* slider = m_slider.getReferencedComponent();
    if (!slider)
    {
        return;
    }

    if (Input::isFaceButtonBottomJustPressed())
    {
        m_currentFill += m_stepAmount;
        if (m_currentFill > 1.0f)
        {
            m_currentFill = 1.0f;
        }

        SliderAPI::setFillAmount(slider, m_currentFill);
        Debug::log("[UISliderTest] Increased fill to %.2f", m_currentFill);
    }

    if (Input::isFaceButtonRightJustPressed())
    {
        m_currentFill -= m_stepAmount;
        if (m_currentFill < 0.0f)
        {
            m_currentFill = 0.0f;
        }

        SliderAPI::setFillAmount(slider, m_currentFill);
        Debug::log("[UISliderTest] Decreased fill to %.2f", m_currentFill);
    }

    if (m_autoTest)
    {
        float delta = Time::getDeltaTime() * m_fillSpeed;

        if (m_increasing)
        {
            m_currentFill += delta;
            if (m_currentFill >= 1.0f)
            {
                m_currentFill = 1.0f;
                m_increasing = false;
            }
        }
        else
        {
            m_currentFill -= delta;
            if (m_currentFill <= 0.0f)
            {
                m_currentFill = 0.0f;
                m_increasing = true;
            }
        }

        SliderAPI::setFillAmount(slider, m_currentFill);
    }
}

IMPLEMENT_SCRIPT(SliderTest)