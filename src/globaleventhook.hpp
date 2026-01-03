#pragma once

#include "globals.hpp"

void registerGlobalEventHook();
void hkOnMouseButton(void* self, SCallbackInfo& info, std::any data);
