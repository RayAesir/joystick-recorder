## Joystick Recorder for Linux

- Emulate joystick from keyboard (autodetected).
- Precisely record your joystick to binary file.
- Play record.

E.g. You can record FPV Controller and use replay in Liftoff.
Use keyboard to calibrate Fake Joystick.

Calibrate Fake Joystick:

https://github.com/user-attachments/assets/b6af1e2d-5f77-47d5-8b0c-47ec54b188e8

Play Record:

https://github.com/user-attachments/assets/18b2bcc7-de90-491a-bf63-ef6451c632e8

## How to use

Emulate joystick:
```console
joystick-recorder --mode emulate
```

**Z/C** - Throttle min/max  
**W/S** - Pitch down/up  
**A/D** - Roll left/right  
**Q/E** - Yaw left/right  
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
