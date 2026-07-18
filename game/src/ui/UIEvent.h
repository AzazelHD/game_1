#pragma once

#include "ui/ActionId.h"
#include "ui/WindowId.h"

#include <string>

enum class UIEventType
{
    ActionSelected,
    ActionCanceled,
    ConfirmResult,
    DialogAdvanced,
    DialogFinished,
    NavigatePrevious,
    NavigateNext,
};

struct UIEvent
{
    UIEventType type = UIEventType::ActionSelected;
    WindowId windowId = WindowId::None;
    ActionId actionId = ActionId::None;
    int index = -1;
    bool confirmed = false;
};
