#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static int fd;

static int64_t realtime_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static void emit(unsigned short type, unsigned short code, int value)
{
    struct input_event event = {.type = type, .code = code, .value = value};
    if (write(fd, &event, sizeof(event)) != sizeof(event))
    {
        perror("write /dev/uinput");
        exit(1);
    }
}

static void report_axes(int x, int y, int rx, int ry)
{
    emit(EV_ABS, ABS_X, x);
    emit(EV_ABS, ABS_Y, y);
    emit(EV_ABS, ABS_RX, rx);
    emit(EV_ABS, ABS_RY, ry);
    emit(EV_SYN, SYN_REPORT, 0);
}

static void sleep_until(struct timespec deadline)
{
    int ret;
    do ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL);
    while (ret == EINTR);
}

static void add_us(struct timespec *ts, long us)
{
    ts->tv_nsec += us * 1000;
    while (ts->tv_nsec >= 1000000000L)
    {
        ts->tv_nsec -= 1000000000L;
        ++ts->tv_sec;
    }
}

int main(int argc, char **argv)
{
    const int trials = argc > 1 ? atoi(argv[1]) : 5;
    const int active_ms = argc > 2 ? atoi(argv[2]) : 2000;
    const int idle_ms = argc > 3 ? atoi(argv[3]) : 4000;
    struct uinput_setup setup = {0};
    struct uinput_abs_setup abs = {0};
    const int axes[] = {ABS_X, ABS_Y, ABS_RX, ABS_RY};
    struct timespec next;
    int trial, sample, i;

    if ((fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
    {
        perror("open /dev/uinput");
        return 1;
    }
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (i = BTN_A; i <= BTN_THUMBR; ++i) ioctl(fd, UI_SET_KEYBIT, i);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    for (i = 0; i < 4; ++i)
    {
        ioctl(fd, UI_SET_ABSBIT, axes[i]);
        memset(&abs, 0, sizeof(abs));
        abs.code = axes[i];
        abs.absinfo.minimum = -32768;
        abs.absinfo.maximum = 32767;
        abs.absinfo.flat = 0;
        abs.absinfo.resolution = 1;
        if (ioctl(fd, UI_ABS_SETUP, &abs) < 0) perror("UI_ABS_SETUP");
    }
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x045e;
    setup.id.product = 0x028e;
    setup.id.version = 0x0114;
    /* Match SDL's built-in Xbox 360 controller mapping. */
    snprintf(setup.name, UINPUT_MAX_NAME_SIZE, "Xbox 360 Controller");
    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0)
    {
        perror("create uinput device");
        return 1;
    }

    fprintf(stderr, "Virtual controller created; allowing discovery for 3 seconds...\n");
    sleep(3);
    printf("event,realtime_us,trial\n");
    fflush(stdout);
    clock_gettime(CLOCK_MONOTONIC, &next);
    for (trial = 1; trial <= trials; ++trial)
    {
        printf("active,%lld,%d\n", (long long)realtime_us(), trial);
        fflush(stdout);
        for (sample = 0; sample < active_ms; ++sample)
        {
            double p = sample * (2.0 * M_PI / 137.0);
            report_axes((int)(28000 * sin(p)), (int)(28000 * cos(p)),
                        (int)(25000 * sin(p * 1.37)), (int)(25000 * cos(p * 1.37)));
            add_us(&next, 1000);
            sleep_until(next);
        }
        report_axes(0, 0, 0, 0);
        printf("neutral,%lld,%d\n", (long long)realtime_us(), trial);
        fflush(stdout);
        usleep(idle_ms * 1000);
        clock_gettime(CLOCK_MONOTONIC, &next);
    }
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
