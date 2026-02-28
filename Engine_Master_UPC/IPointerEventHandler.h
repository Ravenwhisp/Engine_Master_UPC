#include <PointerEventData.h>

class IPointerEventHandler
{
public:
    virtual void onPointerClick(PointerEventData& data) = 0;
    virtual void onPointerUp(PointerEventData& data) = 0;
    virtual ~IPointerEventHandler() = default;
};

