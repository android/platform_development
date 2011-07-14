#include "com_android_commands_monkey_MonkeyProcess.h"

#include <unistd.h>
#include <cutils/log.h>

#define LOG_TAG "monkeyprocess"


JNIEXPORT void
JNICALL Java_com_android_commands_monkey_MonkeyProcess_createNewProcessSession(JNIEnv *, jclass)
{
    pid_t new_pgid = setsid();
    if (new_pgid == (pid_t)-1) {
        ALOGE("Failed to create new session for process %d", getpid());
        ALOGE("Continuing execution with previous setup (pid = %d, ppid = %d, pgrp = %d,"
            "tpgrp = %d)", getpid(), getppid(), getpgrp(), tcgetpgrp(STDIN_FILENO));
    }
    else {
        ALOGV("New process session successfully created for process %d");
    }
}
