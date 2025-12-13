#pragma once
#include "drivers/InterruptManager.h"
#include "config.h"

class FunctionButton {

public:
    FunctionButton(InterruptManager& interruptManager) : interruptManager_(interruptManager) {};
    void update();
private:
    InterruptManager& interruptManager_;
};