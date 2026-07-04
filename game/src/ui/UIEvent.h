#pragma once

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
    std::string windowId;
    std::string actionId;
    int index = -1;
    bool confirmed = false;
};
