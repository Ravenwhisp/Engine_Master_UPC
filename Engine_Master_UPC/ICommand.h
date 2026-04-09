#pragma once

class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void run() = 0;
};
