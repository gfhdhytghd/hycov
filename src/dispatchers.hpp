#pragma once
#include "globals.hpp"

enum class ShiftDirection {
	Left,
	Up,
	Down,
	Right,
};

bool want_auto_fullscren(PHLWINDOW pWindow);
bool isDirectionArg(std::string arg);
PHLWINDOW direction_select(std::string arg);
PHLWINDOW get_circle_next_window (std::string arg);
void warpcursor_and_focus_to_window(PHLWINDOW pWindow);
void switchToLayoutWithoutReleaseData(std::string layout);
void recalculateAllMonitor();

SDispatchResult dispatch_circle(std::string arg);
SDispatchResult dispatch_focusdir(std::string arg);

SDispatchResult dispatch_toggleoverview(std::string arg);
SDispatchResult dispatch_enteroverview(std::string arg);
SDispatchResult dispatch_leaveoverview(std::string arg);

void registerDispatchers();
