#pragma once

namespace device {

void EmulateJoystick();
void RecordJoystick(const char* filename);
void PlayRecord(const char* filename);

}  // namespace device