#include "device.h"

// deps
#include <spdlog/spdlog.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
// global
#include <fcntl.h>
#include <unistd.h>

#include <fstream>
// base evdev
#include <libevdev/libevdev.h>
// evdev wrapper for uinput
#include <libevdev/libevdev-uinput.h>
// local
#include "cmd.h"
#include "global.h"

namespace device {

// support emplace_back()
struct Event {
  Event() = default;
  Event(unsigned short t, unsigned short c, signed int v)
      : type(t), code(c), value(v) {}

  unsigned short type;
  unsigned short code;
  signed int value;

  template <class Archive>
  void serialize(Archive &archive) {
    archive(type, code, value);
  }
};

class Input {
 public:
  Input(const char *event_path) {
    fd_ = open(event_path, O_RDONLY);
    rc_ = libevdev_new_from_fd(fd_, &dev_);
    if (rc_ < 0) {
      spdlog::error("Error cannot open device: {}, at {}", strerror(-rc_),
                    event_path);
      exit(EXIT_FAILURE);
    } else {
      spdlog::info("Input device: {}, at {}", libevdev_get_name(dev_),
                   event_path);
    }
  }
  ~Input() {
    libevdev_free(dev_);
    close(fd_);
  }

  unsigned short KeyPressed() {
    input_event ev;
    rc_ = libevdev_next_event(
        dev_, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
    if (rc_ < 0) {
      spdlog::warn("libevdev_next_event: {}", strerror(-rc_));
      return KEY_RESERVED;
    }

    bool success = (rc_ == LIBEVDEV_READ_STATUS_SUCCESS);
    bool key = (ev.type == EV_KEY);
    bool pressed = (ev.value == 1);
    if (success && key && pressed) {
      return ev.code;
    }

    return KEY_RESERVED;
  }

  input_event ReadJoystick() {
    input_event ev{};
    rc_ = libevdev_next_event(
        dev_, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
    if (rc_ < 0) {
      spdlog::warn("libevdev_next_event: {}", strerror(-rc_));
    }
    return ev;
  }

  void ReadJoystick(std::vector<Event> &events) {
    input_event ev;
    rc_ = libevdev_next_event(
        dev_, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
    if (rc_ < 0) {
      spdlog::warn("libevdev_next_event: {}", strerror(-rc_));
    }

    if (rc_ == LIBEVDEV_READ_STATUS_SUCCESS) {
      if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
        events.emplace_back(ev.type, ev.code,
                            static_cast<signed int>(ev.time.tv_usec));
      } else {
        events.emplace_back(ev.type, ev.code, ev.value);
      }
    }
  }

  // read events with interval
  void ReadSticks(unsigned int ms, int &xl, int &yl, int &xr, int &yr) {
    xl = libevdev_get_event_value(dev_, EV_ABS, ABS_X);
    yl = libevdev_get_event_value(dev_, EV_ABS, ABS_Y);
    xr = libevdev_get_event_value(dev_, EV_ABS, ABS_RX);
    yr = libevdev_get_event_value(dev_, EV_ABS, ABS_RY);
    usleep(ms * 1000);
  }

 private:
  libevdev *dev_{nullptr};
  int fd_;
  int rc_{1};
};

class Joystick {
 public:
  Joystick() {
    dev_ = libevdev_new();
    libevdev_set_name(dev_, "Fake Joystick");

    // enable analog absolute position handling
    libevdev_enable_event_type(dev_, EV_ABS);

    input_absinfo absinfo = {.value = 0,
                             .minimum = kMinSignal,
                             .maximum = kMaxSignal,
                             .fuzz = 0,
                             .flat = 0,
                             .resolution = 1};

    // enable two sticks
    libevdev_enable_event_code(dev_, EV_ABS, ABS_X, &absinfo);
    libevdev_enable_event_code(dev_, EV_ABS, ABS_Y, &absinfo);
    libevdev_enable_event_code(dev_, EV_ABS, ABS_RX, &absinfo);
    libevdev_enable_event_code(dev_, EV_ABS, ABS_RY, &absinfo);

    rc_ = libevdev_uinput_create_from_device(dev_, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                             &uidev_);
    if (rc_ < 0) {
      spdlog::error("Error cannot create from device: {}", strerror(-rc_));
      exit(EXIT_FAILURE);
    }
  }
  ~Joystick() { libevdev_uinput_destroy(uidev_); }

  int Write(int xl, int yl, int xr, int yr) {
    // left stick
    libevdev_uinput_write_event(uidev_, EV_ABS, ABS_X, xl);
    libevdev_uinput_write_event(uidev_, EV_ABS, ABS_Y, yl);
    // right stick
    libevdev_uinput_write_event(uidev_, EV_ABS, ABS_RX, xr);
    libevdev_uinput_write_event(uidev_, EV_ABS, ABS_RY, yr);
    // sync event tells input layer we're done with a "batch" of
    // updates
    rc_ = libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
    return (rc_ < 0) ? rc_ : 0;
  }

  void WriteOne(unsigned short type, unsigned short code, int value) {
    rc_ = libevdev_uinput_write_event(uidev_, type, code, value);
  }

  void Sync() {
    rc_ = libevdev_uinput_write_event(uidev_, EV_SYN, SYN_REPORT, 0);
  }

  void Reset() {
    x_ = 0;
    y_ = kMinSignal;
    x_delta_ = kStepToDeg * 16;
    y_delta_ = kStepToDeg * 16;
    // center sticks
    rc_ = Write(0, 0, 0, 0);
  }

  void RotateSticks() {
    // x and y are triangle wave 90 degrees out of phase
    rc_ = Write(x_, y_, y_, x_);

    x_ += x_delta_;
    y_ += y_delta_;

    if (x_ >= kMaxSignal || x_ <= kMinSignal) x_delta_ = -x_delta_;
    if (y_ >= kMaxSignal || y_ <= kMinSignal) y_delta_ = -y_delta_;
  }

  // left x
  void YawRight() { rc_ = Write(kMaxSignal, 0, 0, 0); }
  void YawLeft() { rc_ = Write(kMinSignal, 0, 0, 0); }
  // left y
  void ThrottleMax() { rc_ = Write(0, kMaxSignal, 0, 0); }
  void ThrottleMin() { rc_ = Write(0, kMinSignal, 0, 0); }
  // right x
  void RollRight() { rc_ = Write(0, 0, kMaxSignal, 0); }
  void RollLeft() { rc_ = Write(0, 0, kMinSignal, 0); }
  // right y
  void PitchDown() { rc_ = Write(0, 0, 0, kMaxSignal); }
  void PitchUp() { rc_ = Write(0, 0, 0, kMinSignal); }

 private:
  const int kMinSignal{-32768};
  const int kMaxSignal{32767};
  // range of 16-bit int divided by stick range
  const int kStepToDeg{(64 * 1024) / 180};
  libevdev *dev_;
  libevdev_uinput *uidev_;
  int rc_;
  // rotation
  int x_{0};
  int y_{kMinSignal};
  int x_delta_{kStepToDeg};
  int y_delta_{kStepToDeg};
};

void EmulateJoystick() {
  const std::string event_path = cmd::QueryKeyboard();
  Input kbd{event_path.c_str()};
  Joystick jsd;

  while (global::IsRunning()) {
    switch (kbd.KeyPressed()) {
      case KEY_Z:
        jsd.ThrottleMin();
        break;
      case KEY_C:
        jsd.ThrottleMax();
        break;

      case KEY_W:
        jsd.PitchDown();
        break;
      case KEY_S:
        jsd.PitchUp();
        break;

      case KEY_A:
        jsd.RollLeft();
        break;
      case KEY_D:
        jsd.RollRight();
        break;

      case KEY_Q:
        jsd.YawLeft();
        break;
      case KEY_E:
        jsd.YawRight();
        break;

      case KEY_F:
        jsd.RotateSticks();
        break;
      case KEY_R:
        jsd.Reset();
        break;

      default:
        // KEY_RESERVED as None or Error
        break;
    }
  }
}

void RecordJoystick(const char *filename) {
  const std::string event_path = cmd::QueryJoystick();
  Input jsd{event_path.c_str()};

  std::ofstream file{filename, std::ios::binary};
  if (file.is_open()) {
    // 8k buffer best choice
    std::array<char, 8192> buf;
    file.rdbuf()->pubsetbuf(buf.data(), buf.size());
    cereal::BinaryOutputArchive archive{file};
    // preallocate some memory
    std::vector<Event> events;
    events.reserve(4096);

    while (global::IsRunning()) {
      // libevdev_next_event() is blocking function
      jsd.ReadJoystick(events);
    }

    // append EV_SYN event
    events.emplace_back(0, 0, 0);
    archive(events);
    file.close();
  } else {
    spdlog::error("Cannot create file: {}", filename);
    exit(EXIT_FAILURE);
  }
}

void PlayRecord(const char *filename) {
  Joystick jsd;

  std::ifstream file{filename, std::ifstream::binary};
  if (file.is_open()) {
    // 8k buffer best choice
    std::array<char, 8192> buf;
    file.rdbuf()->pubsetbuf(buf.data(), buf.size());
    cereal::BinaryInputArchive archive{file};
    // preallocate some memory
    std::vector<Event> events;
    events.reserve(4096);
    archive(events);
    file.close();

    // process events
    for (const auto &ev : events) {
      spdlog::info("type: [{}], code: [{}], value: [{}]", ev.type, ev.code,
                   ev.value);
      // EV_SYN == 0x00
      if (ev.type) {
        jsd.WriteOne(ev.type, ev.code, ev.value);
      } else {
        jsd.Sync();
        usleep(ev.value);
      }
    }
  } else {
    spdlog::error("Cannot open file: {}", filename);
    exit(EXIT_FAILURE);
  }
}

}  // namespace device
