/*    Copyright 2012-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <mtdev.h>
#include <sys/stat.h>

#ifdef ANDROID
#include <android/log.h>
#endif


#define die(args...) do { \
    fprintf(stderr, "ERROR: "); \
    fprintf(stderr, args);   \
    exit(EXIT_FAILURE); \
} while(0)

#define dprintf(args...) if (verbose) printf(args)


#define INPDEV_MAX_DEVICES  16
#define INPDEV_MAX_PATH     30

#ifndef ANDROID
int strlcpy(char *dest, char *source,  size_t size)
{
    strncpy(dest, source, size-1);
    dest[size-1] = '\0';
    return size;
}
#endif

typedef enum {
    FALSE=0,
    TRUE
} bool_t;

typedef enum {
    RECORD=0,
    REPLAY,
    SCALE,
    DUMP,
    INFO,
    INVALID
} revent_mode_t;

typedef struct {
    revent_mode_t mode;
    int32_t record_time;
    int32_t device_number;
    double scale_x, shift_x, scale_y, shift_y;
    char *file, *file2;
} revent_args_t;

typedef struct {
    int32_t id_pathc;                                       /* Count of total paths so far. */
    char   id_pathv[INPDEV_MAX_DEVICES][INPDEV_MAX_PATH];   /* List of paths matching pattern. */
} inpdev_t;

typedef struct {
    int32_t dev_idx;
    int32_t _padding;
    struct input_event event;
} replay_event_t;

typedef struct {
    int32_t num_fds;
    int32_t num_events;
    int *fds;
    replay_event_t *events;
} replay_buffer_t;


bool_t verbose = FALSE;
bool_t translate = FALSE;


bool_t is_numeric(char *string)
{
    int len = strlen(string);

    int i = 0;
    while(i < len)
    {
        if(!isdigit(string[i]))
            return FALSE;
        i++;
    }

    return TRUE;
}

off_t get_file_size(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    die("Cannot determine size of %s: %s\n", filename, strerror(errno));
}

int inpdev_init(inpdev_t **inpdev, int devid)
{
    int32_t i;
    int fd;
    int32_t num_devices;

    *inpdev = malloc(sizeof(inpdev_t));
    (*inpdev)->id_pathc = 0;

    if (devid == -1) {
        // device id was not specified so we want to record from all available input devices.
        for(i = 0; i < INPDEV_MAX_DEVICES; ++i)
        {
            sprintf((*inpdev)->id_pathv[(*inpdev)->id_pathc], "/dev/input/event%d", i);
            fd = open((*inpdev)->id_pathv[(*inpdev)->id_pathc], O_RDONLY);
            if(fd > 0)
            {
                close(fd);
                dprintf("opened %s\n", (*inpdev)->id_pathv[(*inpdev)->id_pathc]);
                (*inpdev)->id_pathc++;
            }
            else
            {
                dprintf("could not open %s\n", (*inpdev)->id_pathv[(*inpdev)->id_pathc]);
            }
        }
    }
    else {
        // device id was specified so record just that device.
        sprintf((*inpdev)->id_pathv[0], "/dev/input/event%d", devid);
        fd = open((*inpdev)->id_pathv[0], O_RDONLY);
        if(fd > 0)
        {
            close(fd);
            dprintf("opened %s\n", (*inpdev)->id_pathv[0]);
            (*inpdev)->id_pathc++;
        }
        else
        {
            die("could not open %s\n", (*inpdev)->id_pathv[0]);
        }
    }

    return 0;
}

int inpdev_close(inpdev_t *inpdev)
{
    free(inpdev);
    return 0;
}

void printDevProperties(const char* aDev)
{
    int fd = -1;
    char name[256]= "Unknown";
    if ((fd = open(aDev, O_RDONLY)) < 0)
        die("could not open %s\n", aDev);

    if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
        die("evdev ioctl failed on %s\n", aDev);

    printf("The device on %s says its name is %s\n",
            aDev, name);
    close(fd);
}

void dump(const char *logfile)
{
    int fdin = open(logfile, O_RDONLY);
    if (fdin < 0) die("Could not open eventlog %s\n", logfile);

    int nfds;
    size_t rb = read(fdin, &nfds, sizeof(nfds));
    printf("read bytes: %zd\n",rb);
    if (rb != sizeof(nfds)) die("problems reading eventlog\n");
    int *fds = malloc(sizeof(int)*nfds);
    if (!fds) die("out of memory\n");

    int32_t len;
    int32_t i;
    char buf[INPDEV_MAX_PATH];

    inpdev_t *inpdev = malloc(sizeof(inpdev_t));
    inpdev->id_pathc = 0;
    for (i=0; i<nfds; i++) {
        memset(buf, 0, sizeof(buf));
        rb = read(fdin, &len, sizeof(len));
        if (rb != sizeof(len)) die("problems reading eventlog\n");
        rb = read(fdin, &buf[0], len);
        if (rb != len) die("problems reading eventlog\n");
        strlcpy(inpdev->id_pathv[inpdev->id_pathc], buf, INPDEV_MAX_PATH);
        inpdev->id_pathv[inpdev->id_pathc][INPDEV_MAX_PATH-1] = '\0';
        inpdev->id_pathc++;
    }

    replay_event_t rep_ev;
    struct input_event ev;
    int count = 0;
    while(1) {
        // Fix a bug that original revent.c doesn't read _padding.
        rb = read(fdin, &rep_ev, sizeof(rep_ev));
        if (rb < sizeof(rep_ev)) break;

        ev = rep_ev.event;

        printf("%10u.%-6u type %d code %d value %d\n",
                (unsigned int)ev.time.tv_sec, (unsigned int)ev.time.tv_usec,
                ev.type, ev.code, ev.value);
        count++;
    }

    printf("\nTotal: %d events\n", count);
    close(fdin);
    free(inpdev);
}

void scale(const char *logfile, double scale_x, double shift_x, double scale_y,
        double shift_y, const char *destfile)
{
    int fdin = open(logfile, O_RDONLY);
    if (fdin < 0) die("Could not open eventlog %s\n", logfile);

    FILE* fdout;
    fdout = fopen(destfile, "wb");

    int nfds;
    size_t rb = read(fdin, &nfds, sizeof(nfds));
    fwrite(&nfds, sizeof(nfds), 1, fdout);
    if (rb != sizeof(nfds)) die("problems reading eventlog\n");
    int *fds = malloc(sizeof(int)*nfds);
    if (!fds) die("out of memory\n");

    int32_t len;
    int32_t i;
    char buf[INPDEV_MAX_PATH];

    inpdev_t *inpdev = malloc(sizeof(inpdev_t));
    inpdev->id_pathc = 0;
    for (i=0; i<nfds; i++) {
        memset(buf, 0, sizeof(buf));
        rb = read(fdin, &len, sizeof(len));
        if (rb != sizeof(len)) die("problems reading eventlog\n");
        fwrite(&len, sizeof(len), 1, fdout);

        rb = read(fdin, &buf[0], len);
        if (rb != len) die("problems reading eventlog\n");
        fwrite(&buf[0], 1, len, fdout);
        strlcpy(inpdev->id_pathv[inpdev->id_pathc], buf, INPDEV_MAX_PATH);
        inpdev->id_pathv[inpdev->id_pathc][INPDEV_MAX_PATH-1] = '\0';
        inpdev->id_pathc++;
    }

    replay_event_t rep_ev;
    struct input_event ev;
    int32_t idx;
    int32_t _padding;
    while(1) {
        rb = read(fdin, &rep_ev, sizeof(rep_ev));
        if (rb < (int)sizeof(rep_ev))
            break;

        ev = rep_ev.event;
        idx = rep_ev.dev_idx;
        _padding = rep_ev._padding;

        if (ev.type == EV_ABS && ev.code == ABS_MT_POSITION_X)
            ev.value = (__s32)((double)ev.value * scale_x) + shift_x;
        if (ev.type == EV_ABS && ev.code == ABS_MT_POSITION_Y)
            ev.value = (__s32)((double)ev.value * scale_y) + shift_y;

        fwrite(&idx, sizeof(idx), 1, fdout);
        fwrite(&_padding, sizeof(_padding), 1, fdout);
        fwrite(&ev, sizeof(ev), 1, fdout);
    }

    close(fdin);
    free(inpdev);
    fclose(fdout);
}

int replay_buffer_init(replay_buffer_t **buffer, const char *logfile, int devid)
{
    *buffer = malloc(sizeof(replay_buffer_t));
    replay_buffer_t *buff = *buffer;
    off_t fsize  = get_file_size(logfile);
    buff->events =  (replay_event_t *)malloc((size_t)fsize);
    if (!buff->events)
        die("out of memory\n");

    int fdin = open(logfile, O_RDONLY);
    if (fdin < 0)
        die("Could not open eventlog %s\n", logfile);

    size_t rb = read(fdin, &(buff->num_fds), sizeof(buff->num_fds));
    if (rb!=sizeof(buff->num_fds))
        die("problems reading eventlog\n");

    buff->fds = malloc(sizeof(int) * buff->num_fds);
    if (!buff->fds)
        die("out of memory\n");

    int32_t len, i;
    char path_buff[256]; // should be more than enough
    for (i = 0; i < buff->num_fds; i++) {
        memset(path_buff, 0, sizeof(path_buff));
        rb = read(fdin, &len, sizeof(len));
        if (rb!=sizeof(len))
            die("problems reading eventlog\n");
        rb = read(fdin, &path_buff[0], len);
        if (rb != len)
            die("problems reading eventlog\n");

        if (devid != -1 && i == 0)
            sprintf(path_buff, "/dev/input/event%d", devid);

        buff->fds[i] = open(path_buff, O_WRONLY | O_NDELAY);
        if (buff->fds[i] < 0)
            die("could not open device file %s\n", path_buff);
    }

    struct timeval start_time;
    replay_event_t rep_ev;
    i = 0;
    while(1) {
        rb = read(fdin, &rep_ev, sizeof(rep_ev));
        if (rb < (int)sizeof(rep_ev))
            break;

        if (i == 0) {
            start_time = rep_ev.event.time;
        }
        timersub(&(rep_ev.event.time), &start_time, &(rep_ev.event.time));
        memcpy(&(buff->events[i]), &rep_ev, sizeof(rep_ev));
        i++;
    }
    buff->num_events = i - 1;
    close(fdin);
    return 0;
}

int replay_buffer_close(replay_buffer_t *buff)
{
    free(buff->fds);
    free(buff->events);
    free(buff);
    return 0;
}

int replay_buffer_play(replay_buffer_t *buff)
{
    int32_t i = 0, rb;
    struct timeval start_time, now, desired_time, last_event_delta, delta;
    memset(&last_event_delta, 0, sizeof(struct timeval));
    gettimeofday(&start_time, NULL);

    while (i < buff->num_events) {
        gettimeofday(&now, NULL);
        timeradd(&start_time, &last_event_delta, &desired_time);

        if (timercmp(&desired_time, &now, >)) {
            timersub(&desired_time, &now, &delta);
            useconds_t d = (useconds_t)delta.tv_sec * 1000000 + delta.tv_usec;
            dprintf("now %u.%u desiredtime %u.%u sleeping %u uS\n",
                    (unsigned int)now.tv_sec, (unsigned int)now.tv_usec,
                    (unsigned int)desired_time.tv_sec, (unsigned int)desired_time.tv_usec, d);
            usleep(d);
        }

        int32_t idx = (buff->events[i]).dev_idx;
        struct input_event ev = (buff->events[i]).event;
        while((i < buff->num_events) && !timercmp(&ev.time, &last_event_delta, !=)) {
            rb = write(buff->fds[idx], &ev, sizeof(ev));
            if (rb!=sizeof(ev))
                die("problems writing\n");
            dprintf("replayed event: type %d code %d value %d\n", ev.type, ev.code, ev.value);

            i++;
            idx = (buff->events[i]).dev_idx;
            ev = (buff->events[i]).event;
        }
        last_event_delta = ev.time;
    }
}

void replay(const char *logfile, int devid)
{
    replay_buffer_t *replay_buffer;
    replay_buffer_init(&replay_buffer, logfile, devid);
#ifdef ANDROID
    __android_log_write(ANDROID_LOG_INFO, "REVENT", "Replay starting");
#endif
    replay_buffer_play(replay_buffer);
#ifdef ANDROID
    __android_log_write(ANDROID_LOG_INFO, "REVENT", "Replay complete");
#endif
    replay_buffer_close(replay_buffer);
}

void record(inpdev_t *inpdev, int delay, const char *logfile)
{
    fd_set readfds;
    FILE* fdout;
    struct input_event ev;
    struct mtdev dev;
    int32_t i;
    int32_t _padding = 0xdeadbeef;
    int32_t maxfd = 0;
    int32_t keydev=0;

    int* fds = malloc(sizeof(int)*inpdev->id_pathc);
    if (!fds) die("out of memory\n");

    fdout = fopen(logfile, "wb");
    if (!fdout) die("Could not open eventlog %s\n", logfile);

    fwrite(&inpdev->id_pathc, sizeof(inpdev->id_pathc), 1, fdout);
    for (i=0; i<inpdev->id_pathc; i++) {
        int32_t len = strlen(inpdev->id_pathv[i]);
        fwrite(&len, sizeof(len), 1, fdout);
        fwrite(inpdev->id_pathv[i], len, 1, fdout);
    }

    for (i=0; i < inpdev->id_pathc; i++)
    {
        fds[i] = open(inpdev->id_pathv[i], O_RDONLY | O_NONBLOCK);
        if (fds[i]>maxfd) maxfd = fds[i];
        dprintf("opened %s with %d\n", inpdev->id_pathv[i], fds[i]);
        if (fds[i]<0) die("could not open \%s\n", inpdev->id_pathv[i]);
    }

    if (translate) {
        // Using translate fd can only be touch device.
        int ret = mtdev_open(&dev, fds[0]);
        if (ret) {
            die("error: could not open device: %d\n", ret);
            return;
        }
    }

    int count =0;
    struct timeval tout;
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        for (i=0; i < inpdev->id_pathc; i++)
            FD_SET(fds[i], &readfds);
        /* wait for input */
        tout.tv_sec = delay;
        tout.tv_usec = 0;
        int32_t r = select(maxfd+1, &readfds, NULL, NULL, &tout);
        /* dprintf("got %d (err %d)\n", r, errno); */
        if (!r) break;
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // in this case the key down for the return key will be recorded
            // so we need to up the key up
            memset(&ev, 0, sizeof(ev));
            ev.type = EV_KEY;
            ev.code = KEY_ENTER;
            ev.value = 0;
            gettimeofday(&ev.time, NULL);
            fwrite(&keydev, sizeof(keydev), 1, fdout);
            fwrite(&_padding, sizeof(_padding), 1, fdout);
            fwrite(&ev, sizeof(ev), 1, fdout);
            memset(&ev, 0, sizeof(ev)); // SYN
            gettimeofday(&ev.time, NULL);
            fwrite(&keydev, sizeof(keydev), 1, fdout);
            fwrite(&_padding, sizeof(_padding), 1, fdout);
            fwrite(&ev, sizeof(ev), 1, fdout);
            dprintf("added fake return exiting...\n");
            break;
        }

        for (i=0; i < inpdev->id_pathc; i++)
        {
            if (FD_ISSET(fds[i], &readfds))
            {
                dprintf("Got event from %s\n", inpdev->id_pathv[i]);
                memset(&ev, 0, sizeof(ev));
                size_t rb;

                if (translate) {
                    if (mtdev_get(&dev, fds[i], &ev, 1) <= 0) continue;
                }
                else {
                    rb = read(fds[i], (void*) &ev, sizeof(ev));
                }

                if (ev.type == EV_KEY && ev.code == KEY_ENTER && ev.value == 1) {
                    keydev = i;
                }

                printf("time: %u %ld-- ", (unsigned int)ev.time.tv_sec,
                        (long int)ev.time.tv_usec);
                printf("%d event: type %d code %d value %d\n",
                        (unsigned int)rb, ev.type, ev.code, ev.value);

                fwrite(&i, sizeof(i), 1, fdout);
                fwrite(&_padding, sizeof(_padding), 1, fdout);
                fwrite(&ev, sizeof(ev), 1, fdout);

                count++;

            }
        }
    }

    for (i=0; i < inpdev->id_pathc; i++)
    {
        close(fds[i]);
    }
    if (translate) {
        mtdev_close(&dev);
    }

    fclose(fdout);
    free(fds);
    printf("Recorded %d events\n", count);
}

void usage()
{
    printf("usage:\n    revent [-h] [-v] COMMAND [OPTIONS] \n"
            "\n"
            "    Options:\n"
            "        -h  print this help message and quit.\n"
            "        -v  enable verbose output.\n"
            "        -r  translate input events to type B protocol when recording.\n"
            "\n"
            "    Commands:\n"
            "        record [-t SECONDS] [-d DEVICE] [-r] FILE\n"
            "            Record input event. stops after return on STDIN (or, optionally, \n"
            "            a fixed delay)\n"
            "\n"
            "                FILE       file into which events will be recorded.\n"
            "                -t SECONDS time, in seconds, for which to record events.\n"
            "                           if not specifed, recording will continue until\n"
            "                           return key is pressed.\n"
            "                -d DEVICE  the number of the input device form which\n"
            "                           events will be recoreded. If not specified, \n"
            "                           all available inputs will be used.\n"
            "\n"
            "        replay FILE\n"
            "            replays previously recorded events from the specified file.\n"
            "\n"
            "                FILE       file into which events will be recorded.\n"
            "                -d DEVICE  the number of the input device form which\n"
            "                           events will be replayed. If not specified, \n"
            "                           all available inputs will be used.\n"
            "\n"
            "        scale FILE1 A1 B1 A2 B2 FILE2\n"
            "            scales previously recorded events from a specified file.\n"
            "\n"
            "                FILE1      file from which events will be read from.\n"
            "                A1, A2     x and y scale factors.\n"
            "                B1, B2     x and y offsets.\n"
            "                FILE2      file into which scaled events will be written.\n"
            "\n"
            "        dump FILE\n"
            "            dumps the contents of the specified event log to STDOUT in\n"
            "            human-readable form.\n"
            "\n"
            "                FILE       event log which will be dumped.\n"
            "\n"
            "        info\n"
            "             shows info about each event char device\n"
            "\n"
            );
}

void add_virtual_device(int *uinp_fd)
{
    struct uinput_user_dev uinp;

    *uinp_fd = open("/dev/uinput", O_WRONLY|O_NONBLOCK|O_NDELAY);
    if(*uinp_fd <= 0) {
        die("Unable to open /dev/uinput\n");
        return ;
    }

    // configure touch device event properties
    memset(&uinp, 0, sizeof(uinp));
    strlcpy(uinp.name, "MyTouchScreen", UINPUT_MAX_NAME_SIZE);
    uinp.id.vendor = 1;
    uinp.id.product = 1;
    uinp.id.version = 1;
    uinp.id.bustype = 0;
    uinp.absmin[ABS_MT_SLOT] = 0;
    uinp.absmax[ABS_MT_SLOT] = 9; // track up to 9 fingers
    uinp.absmin[ABS_MT_TOUCH_MAJOR] = 0;
    uinp.absmax[ABS_MT_TOUCH_MAJOR] = 47;
    uinp.absmin[ABS_MT_TOUCH_MINOR] = 0;
    uinp.absmax[ABS_MT_TOUCH_MINOR] = 47;
    //ABS_MT_POSITION has to be same as the real input device
    uinp.absmin[ABS_MT_POSITION_X] = 0;
    uinp.absmax[ABS_MT_POSITION_X] = 3072;
    uinp.absmin[ABS_MT_POSITION_Y] = 0;
    uinp.absmax[ABS_MT_POSITION_Y] = 2304;
    uinp.absmin[ABS_MT_TRACKING_ID] = 0;
    uinp.absmax[ABS_MT_TRACKING_ID] = 65535;
    uinp.absmin[ABS_MT_PRESSURE] = 0;
    uinp.absmax[ABS_MT_PRESSURE] = 255;

    if (ioctl(*uinp_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT) < 0
            || ioctl(*uinp_fd, UI_SET_EVBIT, EV_SYN) < 0
            || ioctl(*uinp_fd, UI_SET_EVBIT, EV_ABS) < 0
            || ioctl(*uinp_fd, UI_SET_EVBIT, EV_KEY) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_SLOT) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MINOR) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_POSITION_X) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y) < 0
            || ioctl(*uinp_fd, UI_SET_ABSBIT, ABS_MT_PRESSURE) < 0) {
        close(*uinp_fd);
        die("Could not configure virtual touch device.\n");
        return ;
    }

    int res = write(*uinp_fd, &uinp, sizeof(uinp));
    if (res < 0) {
        close(*uinp_fd);
        die("Could not write virtual device info: %d\n", errno);
        return ;
    }
    if (ioctl(*uinp_fd, UI_DEV_CREATE) < 0) {
        close(*uinp_fd);
        die("Could not create virtual device.\n");
        return ;
    }
}

void revent_args_init(revent_args_t **rargs, int argc, char** argv)
{
    *rargs = malloc(sizeof(revent_args_t));
    revent_args_t *revent_args = *rargs;
    revent_args->mode = INVALID;
    revent_args->record_time = INT_MAX;
    revent_args->device_number = -1;
    revent_args->file = NULL;
    revent_args->file2 = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "ht:d:vr")) != -1)
    {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            case 't':
                if (is_numeric(optarg)) {
                    revent_args->record_time = atoi(optarg);
                    dprintf("timeout: %d\n", revent_args->record_time);
                } else {
                    die("-t parameter must be numeric; got %s.\n", optarg);
                }
                break;
            case 'd':
                if (is_numeric(optarg)) {
                    revent_args->device_number = atoi(optarg);
                    dprintf("device: %d\n", revent_args->device_number);
                } else {
                    die("-d parameter must be numeric; got %s.\n", optarg);
                }
                break;
            case 'v':
                verbose = TRUE;
                break;
            case 'r':
                translate = TRUE;
                break;
            default:
                die("Unexpected option: %c", opt);
        }
    }

    int next_arg = optind;
    if (next_arg == argc) {
        usage();
        die("Must specify a command.\n");
    }
    if (!strcmp(argv[next_arg], "record"))
        revent_args->mode = RECORD;
    else if (!strcmp(argv[next_arg], "replay"))
        revent_args->mode = REPLAY;
    else if(!strcmp(argv[next_arg], "scale"))
        revent_args->mode = SCALE;
    else if (!strcmp(argv[next_arg], "dump"))
        revent_args->mode = DUMP;
    else if (!strcmp(argv[next_arg], "info"))
        revent_args->mode = INFO;
    else {
        usage();
        die("Unknown command -- %s\n", argv[next_arg]);
    }
    next_arg++;

    if (next_arg != argc) {
        revent_args->file = argv[next_arg];
        dprintf("file: %s\n", revent_args->file);
        next_arg++;
        if(revent_args->mode == SCALE){
            if(next_arg + 4 == argc) {
                die("Must specify parameter and destination file for scaling"
                        " events. (use -h for ehlp)\n");
            }
            revent_args->scale_x = atof(argv[next_arg]);
            next_arg++;
            revent_args->shift_x = atof(argv[next_arg]);
            next_arg++;
            revent_args->scale_y = atof(argv[next_arg]);
            next_arg++;
            revent_args->shift_y = atof(argv[next_arg]);
            next_arg++;
            revent_args->file2 = argv[next_arg];
            next_arg++;
            dprintf("file2: %s\n", revent_args->file2);
        }
        if (next_arg != argc) {
            die("Trailling arguments (use -h for help).\n");
        }
    }

    if ((revent_args->mode != RECORD) && (revent_args->record_time != INT_MAX)) {
        die("-t parameter is only valid for \"record\" command.\n");
    }
    if ((revent_args->mode != RECORD && revent_args->mode != REPLAY) &&
            (revent_args->device_number != -1)) {
        die("-d parameter is only valid for \"record\" or \"replay\" command.\n");
    }
    if (revent_args->mode != RECORD && translate == TRUE) {
        die("-r is only valid for \"record\" command.\n");
    }
    if ((revent_args->mode == INFO) && (revent_args->file != NULL)) {
        die("File path cannot be specified for \"info\" command.\n");
    }
    if (((revent_args->mode == RECORD) || (revent_args->mode == REPLAY)) &&
            (revent_args->file == NULL)) {
        die("Must specify a file for recording/replaying (use -h for help).\n");
    }
    if (revent_args->mode == RECORD && translate == TRUE && revent_args->device_number == -1) {
        die("Must specify a touch device for translate (use -h for help).\n");
    }
}

int revent_args_close(revent_args_t *rargs)
{
    free(rargs);
    return 0;
}

int main(int argc, char** argv)
{
    int i;
    char *logfile = NULL;

    revent_args_t *rargs;
    revent_args_init(&rargs, argc, argv);

    int uinp_fd;
    add_virtual_device(&uinp_fd);

    inpdev_t *inpdev;
    inpdev_init(&inpdev, rargs->device_number);

    switch(rargs->mode) {
        case RECORD:
            printf("start record\n");
            record(inpdev, rargs->record_time, rargs->file);
            break;
        case REPLAY:
            replay(rargs->file, rargs->device_number);
            break;
        case SCALE:
            scale(rargs->file, rargs->scale_x, rargs->shift_x, rargs->scale_y, rargs->shift_y,
                    rargs->file2);
            break;
        case DUMP:
            dump(rargs->file);
            break;
        case INFO:
            for (i = 0; i < inpdev->id_pathc; i++) {
                printDevProperties(inpdev->id_pathv[i]);
            }
    };

    inpdev_close(inpdev);
    close(uinp_fd);
    revent_args_close(rargs);
    return 0;
}

