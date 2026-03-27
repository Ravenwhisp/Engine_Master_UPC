#pragma once

enum class FillMethod
{
    Horizontal,
    Vertical,
    Radial90,
    Radial180,
    Radial360
};

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

    Radial180Bottom = 0,
    Radial180Left = 1,
    Radial180Top = 2,
    Radial180Right = 3,

    Radial360Clockwise = 0,
    Radial360CounterClockwise = 1
};
