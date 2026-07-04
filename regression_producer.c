#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <math.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

struct pad { int fd, number; };

static int64_t now_us(void) { struct timespec t; clock_gettime(CLOCK_REALTIME,&t); return (int64_t)t.tv_sec*1000000+t.tv_nsec/1000; }
static void log_event(const char *event, int pad, int value) { printf("%s,%lld,%d,%d\n",event,(long long)now_us(),pad,value); fflush(stdout); }
static void emit(struct pad *p, int type, int code, int value) { struct input_event e={.type=type,.code=code,.value=value}; if(write(p->fd,&e,sizeof(e))!=(ssize_t)sizeof(e)){perror("uinput write");exit(1);} }
static void sync_pad(struct pad *p) { emit(p,EV_SYN,SYN_REPORT,0); }
static void key(struct pad *p, int code, int value, const char *name) { emit(p,EV_KEY,code,value); sync_pad(p); log_event(name,p->number,value); }
static void axis(struct pad *p,int x,int y,int rx,int ry){emit(p,EV_ABS,ABS_X,x);emit(p,EV_ABS,ABS_Y,y);emit(p,EV_ABS,ABS_RX,rx);emit(p,EV_ABS,ABS_RY,ry);sync_pad(p);}
static void abs_setup(int fd,int code,int min,int max){struct uinput_abs_setup a={.code=code};a.absinfo.minimum=min;a.absinfo.maximum=max;a.absinfo.resolution=1;ioctl(fd,UI_SET_ABSBIT,code);if(ioctl(fd,UI_ABS_SETUP,&a)<0){perror("UI_ABS_SETUP");exit(1);}}

static struct pad create_pad(int number)
{
    struct pad p={.fd=open("/dev/uinput",O_RDWR|O_NONBLOCK),.number=number};
    struct uinput_setup s={0}; int i;
    if(p.fd<0){perror("/dev/uinput");exit(1);}
    ioctl(p.fd,UI_SET_EVBIT,EV_KEY); for(i=BTN_A;i<=BTN_THUMBR;i++) ioctl(p.fd,UI_SET_KEYBIT,i);
    ioctl(p.fd,UI_SET_EVBIT,EV_ABS);
    abs_setup(p.fd,ABS_X,-32768,32767); abs_setup(p.fd,ABS_Y,-32768,32767);
    abs_setup(p.fd,ABS_RX,-32768,32767); abs_setup(p.fd,ABS_RY,-32768,32767);
    abs_setup(p.fd,ABS_Z,0,255); abs_setup(p.fd,ABS_RZ,0,255);
    abs_setup(p.fd,ABS_HAT0X,-1,1); abs_setup(p.fd,ABS_HAT0Y,-1,1);
    ioctl(p.fd,UI_SET_EVBIT,EV_FF); ioctl(p.fd,UI_SET_FFBIT,FF_RUMBLE);
    s.ff_effects_max=16; s.id.bustype=BUS_USB; s.id.vendor=0x045e; s.id.product=0x028e; s.id.version=0x0114;
    snprintf(s.name,sizeof(s.name),"Xbox 360 Controller");
    if(ioctl(p.fd,UI_DEV_SETUP,&s)<0||ioctl(p.fd,UI_DEV_CREATE)<0){perror("UI_DEV_CREATE");exit(1);}
    return p;
}

static void pump_ff(struct pad *p)
{
    struct input_event e;
    while(read(p->fd,&e,sizeof(e))==(ssize_t)sizeof(e))
    {
        if(e.type==EV_UINPUT&&e.code==UI_FF_UPLOAD)
        {
            struct uinput_ff_upload u={.request_id=e.value};
            if(ioctl(p->fd,UI_BEGIN_FF_UPLOAD,&u)==0){log_event("ff_upload",p->number,(u.effect.u.rumble.strong_magnitude<<16)|u.effect.u.rumble.weak_magnitude);u.retval=0;ioctl(p->fd,UI_END_FF_UPLOAD,&u);}
        }
        else if(e.type==EV_UINPUT&&e.code==UI_FF_ERASE)
        {
            struct uinput_ff_erase x={.request_id=e.value}; if(ioctl(p->fd,UI_BEGIN_FF_ERASE,&x)==0){x.retval=0;ioctl(p->fd,UI_END_FF_ERASE,&x);}
        }
        else if(e.type==EV_FF) log_event(e.value?"ff_play":"ff_stop",p->number,e.code);
    }
}

static void msleep(int ms){struct timespec t={.tv_sec=ms/1000,.tv_nsec=(ms%1000)*1000000L};while(nanosleep(&t,&t)<0&&errno==EINTR);}

int main(int argc, char **argv)
{
    int rate=argc>1?atoi(argv[1]):1000,samples=5*rate,period_us=1000000/rate;
    struct pad p1=create_pad(1),p2=create_pad(2); int n;
    puts("event,realtime_us,pad,value"); fflush(stdout); log_event("created",1,0);log_event("created",2,0);
    for(n=0;n<7000;n++){pump_ff(&p1);pump_ff(&p2);usleep(1000);}
    key(&p1,BTN_A,1,"a");msleep(80);key(&p1,BTN_A,0,"a");
    key(&p2,BTN_B,1,"b");msleep(80);key(&p2,BTN_B,0,"b");
    for(n=0;n<samples;n++)
    {
        double q=n*(2*M_PI/137.0); axis(&p1,28000*sin(q),28000*cos(q),25000*sin(q*1.3),25000*cos(q*1.3));
        axis(&p2,22000*cos(q*.9),22000*sin(q*.9),20000*cos(q*1.1),20000*sin(q*1.1));
        if(n==400*rate/1000) key(&p1,BTN_X,1,"x");
        if(n==440*rate/1000) key(&p1,BTN_X,0,"x");
        if(n==700*rate/1000) key(&p2,BTN_Y,1,"y");
        if(n==740*rate/1000) key(&p2,BTN_Y,0,"y");
        if(n==1000*rate/1000){emit(&p1,EV_ABS,ABS_HAT0X,1);sync_pad(&p1);log_event("dpad_right",1,1);}
        if(n==1040*rate/1000){emit(&p1,EV_ABS,ABS_HAT0X,0);sync_pad(&p1);log_event("dpad_right",1,0);}
        if(n==1400*rate/1000){emit(&p2,EV_ABS,ABS_HAT0Y,-1);sync_pad(&p2);log_event("dpad_up",2,1);}
        if(n==1440*rate/1000){emit(&p2,EV_ABS,ABS_HAT0Y,0);sync_pad(&p2);log_event("dpad_up",2,0);}
        if(n==1800*rate/1000){emit(&p1,EV_ABS,ABS_Z,255);sync_pad(&p1);log_event("lt",1,255);}
        if(n==1900*rate/1000){emit(&p1,EV_ABS,ABS_Z,0);sync_pad(&p1);log_event("lt",1,0);}
        if(n==2200*rate/1000){emit(&p2,EV_ABS,ABS_RZ,255);sync_pad(&p2);log_event("rt",2,255);}
        if(n==2300*rate/1000){emit(&p2,EV_ABS,ABS_RZ,0);sync_pad(&p2);log_event("rt",2,0);}
        pump_ff(&p1);pump_ff(&p2);usleep(period_us);
    }
    axis(&p1,0,0,0,0);axis(&p2,0,0,0,0);log_event("neutral",1,0);log_event("neutral",2,0);
    for(n=0;n<2000;n++){pump_ff(&p1);pump_ff(&p2);usleep(1000);}
    ioctl(p1.fd,UI_DEV_DESTROY);close(p1.fd);log_event("removed",1,0);msleep(1000);
    p1=create_pad(1);log_event("recreated",1,0);msleep(2000);key(&p1,BTN_A,1,"reconnect_a");msleep(80);key(&p1,BTN_A,0,"reconnect_a");
    for(n=0;n<1000;n++){pump_ff(&p1);pump_ff(&p2);usleep(1000);}
    ioctl(p1.fd,UI_DEV_DESTROY);ioctl(p2.fd,UI_DEV_DESTROY);close(p1.fd);close(p2.fd);return 0;
}
