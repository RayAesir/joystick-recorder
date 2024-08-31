## Joystick Recorder for Linux

- Emulate joystick from keyboard (autodetected).
- Precisely record your joystick to binary file.
- Play record.

E.g. You can even record FPV Controller and use replay in Liftoff.
Use keyboard to calibrate Fake Joystick.

![Calibrate Fake Joystick](./docs/calibrate.png 'Calibrate Fake Joystick')

![Play Record](./docs/play_record.png 'Play Record')

## How to use

Emulate joystick:
```console
joystick-recorder --mode emulate
```

**Z** - Throttle min
**C** - Throttle max

**W** - Pitch down
**S** - Pitch up

**A** - Roll left
**D** - Roll right

**Q** - Yaw left
**E** - Yaw right

**F** - Rotate sticks
**R** - Reset

Record:
```console
joystick-recorder --mode record --file name
```

Play replay:
```console
joystick-recorder --mode play --file name
```

Screen capture:
```console
joystick-recorder --mode screen-capture
```

## Building

Install [vcpkg](https://github.com/microsoft/vcpkg).

```Console
cd joystick-recorder
cmake -B build -S . --preset release-x64-linux
```

Reccomended to use VSCode with CMake Tools extension.

## TODO

- Screen capture for YOLO (FFMPEG and v4l2loopback) to control Liftoff drone.

## Dependencies

Logger: [spdlog](https://github.com/gabime/spdlog)  
Serialize: [cereal](https://github.com/USCiLab/cereal)  
Console: [CLI11](https://github.com/CLIUtils/CLI11)  
