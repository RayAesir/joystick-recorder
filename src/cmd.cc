#include "cmd.h"

// deps
#include <spdlog/spdlog.h>
// fork, exec
#include <unistd.h>
// kill child process
#include <signal.h>
#include <sys/prctl.h>
// local
#include "global.h"

namespace cmd {

std::string ExecuteCommand(const char* cmd) {
  constexpr int kMaxBuffer = 256;
  char buffer[kMaxBuffer];
  std::string data{""};

  FILE* pipe = popen(cmd, "r");
  if (!pipe) {
    spdlog::error("Couldn't execute command: {}", cmd);
  } else {
    while (!feof(pipe)) {
      if (fgets(buffer, kMaxBuffer, pipe) != NULL) {
        data.append(buffer);
      }
    }
    pclose(pipe);
  }

  return data;
}

std::string QueryKeyboard() {
  static const std::string kGetInputDeviceEvent{
      "grep -E 'Handlers|EV=' /proc/bus/input/devices |"
      "grep -B1 'EV=120013' |"
      "grep -Eo 'event[0-9]+' |"
      "grep -Eo '[0-9]+' |"
      "tr -d '\n'"};

  std::string dev{"/dev/input/event"};
  std::string event_number = ExecuteCommand(kGetInputDeviceEvent.c_str());
  dev.append(event_number);
  return dev;
}

std::string QueryJoystick() {
  // maybe like this:
  // "grep -B1 'js0' |"
  static const std::string kGetInputDeviceEvent{
      "grep -E 'Handlers|EV=' /proc/bus/input/devices |"
      "grep -B1 'EV=9' |"
      "grep -Eo 'event[0-9]+' |"
      "grep -Eo '[0-9]+' |"
      "tr -d '\n'"};

  std::string dev{"/dev/input/event"};
  std::string event_number = ExecuteCommand(kGetInputDeviceEvent.c_str());
  dev.append(event_number);
  return dev;
}

void StartVideoCapture() {
  // for exec*p fullpath and process name the same
  // v - vector (array as one arg), l - list (many string as args)
  const char* kProcess{"ffmpeg"};
  // https://trac.ffmpeg.org/wiki/Encode/H.264
  char* kArgs[]{
      (char*)"ffmpeg",         //
      (char*)"-f",             // record screen
      (char*)"x11grab",        //
      (char*)"-framerate",     // fps
      (char*)"60",             //
      (char*)"-i",             // x11grab offset
      (char*)":0.0+0,0",       //
      (char*)"-vf",            // rescale for YOLO
      (char*)"scale=640:640",  //
      (char*)"-b:v",           // bitrate for video 640x640x60x24
      (char*)"7500k",          //
      (char*)"-crf",           // quality 0 is loseless
      (char*)"0",              //

      (char*)"-c:v",         // support RGB format (default YUV family)
      (char*)"libx264rgb",   //
      (char*)"-pix_fmt",     // set OpenCV format
      (char*)"bgr24",        //
      (char*)"-profile:v",   // enable rgb profile (main not work)
      (char*)"high444",      //
      (char*)"-preset",      // speedup
      (char*)"fast",         //
      (char*)"-tune",        // preset for streaming
      (char*)"zerolatency",  //

      (char*)"-f",           // output to virtual camera
      (char*)"v4l2",         // require 'sudo modprobe v4l2loopback'
      (char*)"/dev/video0",  //
      //   (char*)"output.mp4",  //

      (char*)NULL  //
  };

  pid_t pid_before_fork = getpid();
  pid_t pid;
  pid = fork();
  // first child pid == 0
  if (pid == 0) {
    // install a parent death signal in the child
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
    if (r == -1) {
      perror(0);
      exit(EXIT_FAILURE);
    }
    // in case the original parent exited just before
    // the prctl() call
    if (getppid() != pid_before_fork) {
      exit(EXIT_FAILURE);
    }
    execvp(kProcess, kArgs);
  }
  // parent
  spdlog::info("parent process: {}", pid);
  while (global::IsRunning()) {
    sleep(1);
  }
}

}  // namespace cmd