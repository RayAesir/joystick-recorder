// deps
#include <spdlog/spdlog.h>

#include <CLI/CLI.hpp>
// local
#include "cmd.h"
#include "device.h"
#include "global.h"

/*
path: /dev/hidraw0
https://www.kernel.org/doc/Documentation/hid/hidraw.txt

HIDRAW - Raw Access to USB and Bluetooth Human Interface Devices
     ==================================================================

The hidraw driver provides a raw interface to USB and Bluetooth Human
Interface Devices (HIDs).  It differs from hiddev in that reports sent and
received are not parsed by the HID parser, but are sent to and received from
the device unmodified.

Hidraw should be used if the userspace application knows exactly how to
communicate with the hardware device, and is able to construct the HID
reports manually.  This is often the case when making userspace drivers for
custom HID devices.

Hidraw is also useful for communicating with non-conformant HID devices
which send and receive data in a way that is inconsistent with their report
descriptors.  Because hiddev parses reports which are sent and received
through it, checking them against the device's report descriptor, such
communication with these non-conformant devices is impossible using hiddev.
Hidraw is the only alternative, short of writing a custom kernel driver, for
these non-conformant devices.

A benefit of hidraw is that its use by userspace applications is independent
of the underlying hardware type.  Currently, Hidraw is implemented for USB
and Bluetooth.  In the future, as new hardware bus types are developed which
use the HID specification, hidraw will be expanded to add support for these
new bus types.

Hidraw uses a dynamic major number, meaning that udev should be relied on to
create hidraw device nodes.  Udev will typically create the device nodes
directly under /dev (eg: /dev/hidraw0).  As this location is distribution-
and udev rule-dependent, applications should use libudev to locate hidraw
devices attached to the system.  There is a tutorial on libudev with a
working example at:
        http://www.signal11.us/oss/udev/
*/

/*
path: /dev/usb/hiddev0
https://www.kernel.org/doc/Documentation/hid/hiddev.txt

In addition to the normal input type HID devices, USB also uses the
human interface device protocols for things that are not really human
interfaces, but have similar sorts of communication needs. The two big
examples for this are power devices (especially uninterruptable power
supplies) and monitor control on higher end monitors.

To support these disparate requirements, the Linux USB system provides
HID events to two separate interfaces:
* the input subsystem, which converts HID events into normal input
device interfaces (such as keyboard, mouse and joystick) and a
normalised event interface - see Documentation/input/input.rst
* the hiddev interface, which provides fairly raw HID events

The data flow for a HID event produced by a device is something like
the following :

 usb.c ---> hid-core.c  ----> hid-input.c ----> [keyboard/mouse/joystick/event]
                         |
                         |
                          --> hiddev.c ----> POWER / MONITOR CONTROL

In addition, other subsystems (apart from USB) can potentially feed
events into the input subsystem, but these have no effect on the hid
device interface.
*/

/*
path: /dev/input/js0
Joystick API (first one, the oldest)
package: joyutils aka joydev (based on sdl2), hidapi with libusb as Linux
backend command: udevadm part of systemd
*/

/*
path: /dev/uinput

Better to create device with libevdev (wrapper over uinput).

uinput is a kernel module that makes it possible to emulate input devices from
userspace. By writing to /dev/uinput (or /dev/input/uinput) device, a process
can create a virtual input device with specific capabilities. Once this virtual
device is created, the process can send events through it, that will be
delivered to userspace and in-kernel consumers.
*/

/*
path: /dev/input/by-id/usb-device-name-event-type
evdev API (kernel interface, newer)
package: livevdev
command: udevadm part of systemd
*/

enum class Mode : int {
  kEmulateJoystick,
  kRecordJoystick,
  kPlayRecord,
  kScreenCapture,
};

const std::map<std::string, Mode> kModeMap = {
    {"emulate", Mode::kEmulateJoystick},       //
    {"record", Mode::kRecordJoystick},         //
    {"play", Mode::kPlayRecord},               //
    {"screen-capture", Mode::kScreenCapture},  //
};

int main(int argc, char **argv) {
  spdlog::set_pattern("%^[%l]%$ %v");

  CLI::App app{"Joystick Recorder and Emulator"};

  Mode mode = Mode::kEmulateJoystick;
  app.add_option("--mode", mode, "Set program mode")
      ->required()
      ->transform(CLI::CheckedTransformer(kModeMap, CLI::ignore_case));

  std::string record_name = "record";
  app.add_option("--file", record_name, "The filename passed to Recorder");

  CLI11_PARSE(app, argc, argv);

  global::CatchSignals();

  switch (mode) {
    case Mode::kEmulateJoystick:
      device::EmulateJoystick();
      break;
    case Mode::kRecordJoystick:
      device::RecordJoystick(record_name.c_str());
      break;
    case Mode::kPlayRecord:
      device::PlayRecord(record_name.c_str());
      break;
    case Mode::kScreenCapture: {
      cmd::StartVideoCapture();
    } break;

    default:
      break;
  }

  return 0;
}
