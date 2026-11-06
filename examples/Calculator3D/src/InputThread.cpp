#include "Input.hpp"

void* APS5_VABI Input::vibrationEntry(void* argument) { return static_cast<Input*>(argument)->runVibrationThread(); }
