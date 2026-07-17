#pragma once
#include <cstdint>
#include <cstring>

enum class FillMethod
{
    Horizontal,
    Vertical,
    Radial90,
    Radial180,
    Radial360
};

inline const char* FillMethodToString(uint32_t v)
{
    switch (static_cast<FillMethod>(v))
    {
    case FillMethod::Horizontal: return "Horizontal";
    case FillMethod::Vertical:   return "Vertical";
    case FillMethod::Radial90:   return "Radial90";
    case FillMethod::Radial180:  return "Radial180";
    case FillMethod::Radial360:  return "Radial360";
    default: return "Horizontal";
    }
}

inline uint32_t StringToFillMethod(const char* s)
{
    if (std::strcmp(s, "Horizontal") == 0) return 0;
    if (std::strcmp(s, "Vertical") == 0)   return 1;
    if (std::strcmp(s, "Radial90") == 0)   return 2;
    if (std::strcmp(s, "Radial180") == 0)  return 3;
    if (std::strcmp(s, "Radial360") == 0)  return 4;
    return 0;
}

enum class FillOrigin
{
    HorizontalLeft = 0,
    HorizontalRight = 1,

    VerticalBottom = 0,
    VerticalTop = 1,

    Radial90BottomLeft = 0,
    Radial90TopLeft = 1,
    Radial90TopRight = 2,
    Radial90BottomRight = 3,
    Radial90BottomLeftCCW = 4,
    Radial90TopLeftCCW = 5,
    Radial90TopRightCCW = 6,
    Radial90BottomRightCCW = 7,

    Radial180Bottom = 0,
    Radial180Left = 1,
    Radial180Top = 2,
    Radial180Right = 3,
    Radial180BottomCCW = 4,
    Radial180LeftCCW = 5,
    Radial180TopCCW = 6,
    Radial180RightCCW = 7,

    Radial360Clockwise = 0,
    Radial360CounterClockwise = 1
};

inline const char* FillOriginToString(uint32_t v)
{
    switch (v)
    {
    case 0:  return "HorizontalLeft";
    case 1:  return "HorizontalRight";
    case 2:  return "Radial90TopRight";
    case 3:  return "Radial90BottomRight";
    case 4:  return "Radial90BottomLeftCCW";
    case 5:  return "Radial90TopLeftCCW";
    case 6:  return "Radial90TopRightCCW";
    case 7:  return "Radial90BottomRightCCW";
    default: return "HorizontalLeft";
    }
}

inline uint32_t StringToFillOrigin(const char* s)
{
    if (std::strcmp(s, "HorizontalLeft") == 0)         return 0;
    if (std::strcmp(s, "HorizontalRight") == 0)        return 1;
    if (std::strcmp(s, "VerticalBottom") == 0)          return 0;
    if (std::strcmp(s, "VerticalTop") == 0)             return 1;
    if (std::strcmp(s, "Radial90BottomLeft") == 0)      return 0;
    if (std::strcmp(s, "Radial90TopLeft") == 0)         return 1;
    if (std::strcmp(s, "Radial90TopRight") == 0)        return 2;
    if (std::strcmp(s, "Radial90BottomRight") == 0)     return 3;
    if (std::strcmp(s, "Radial90BottomLeftCCW") == 0)   return 4;
    if (std::strcmp(s, "Radial90TopLeftCCW") == 0)      return 5;
    if (std::strcmp(s, "Radial90TopRightCCW") == 0)     return 6;
    if (std::strcmp(s, "Radial90BottomRightCCW") == 0)  return 7;
    if (std::strcmp(s, "Radial180Bottom") == 0)         return 0;
    if (std::strcmp(s, "Radial180Left") == 0)           return 1;
    if (std::strcmp(s, "Radial180Top") == 0)            return 2;
    if (std::strcmp(s, "Radial180Right") == 0)          return 3;
    if (std::strcmp(s, "Radial180BottomCCW") == 0)      return 4;
    if (std::strcmp(s, "Radial180LeftCCW") == 0)        return 5;
    if (std::strcmp(s, "Radial180TopCCW") == 0)         return 6;
    if (std::strcmp(s, "Radial180RightCCW") == 0)       return 7;
    if (std::strcmp(s, "Radial360Clockwise") == 0)      return 0;
    if (std::strcmp(s, "Radial360CounterClockwise") == 0) return 1;
    return 0;
}
