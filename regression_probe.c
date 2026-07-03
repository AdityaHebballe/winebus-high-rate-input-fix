#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t now_us(void){FILETIME f;ULARGE_INTEGER u;GetSystemTimePreciseAsFileTime(&f);u.LowPart=f.dwLowDateTime;u.HighPart=f.dwHighDateTime;return(u.QuadPart-116444736000000000ULL)/10;}

int main(int argc,char **argv)
{
    const char *states=argc>1?argv[1]:"states.csv",*rumble=argc>2?argv[2]:"rumble.csv";
    DWORD duration=argc>3?strtoul(argv[3],0,10):24000,start=GetTickCount(),last[4]={~0u,~0u,~0u,~0u},present[4]={0};
    int sent_start=0,sent_stop=0; FILE *s=fopen(states,"w"),*r=fopen(rumble,"w");
    if(!s||!r)return 2;
    fprintf(s,"realtime_us,slot,connected,packet,lx,ly,rx,ry,lt,rt,buttons\n");
    fprintf(r,"realtime_us,slot,action,result\n");
    while(GetTickCount()-start<duration)
    {
        DWORD elapsed=GetTickCount()-start;
        for(DWORD slot=0;slot<4;slot++)
        {
            XINPUT_STATE x={0};DWORD ret=XInputGetState(slot,&x);
            if(ret==ERROR_SUCCESS&&(!present[slot]||x.dwPacketNumber!=last[slot]))
            {
                fprintf(s,"%llu,%lu,1,%lu,%d,%d,%d,%d,%u,%u,%u\n",(unsigned long long)now_us(),(unsigned long)slot,(unsigned long)x.dwPacketNumber,x.Gamepad.sThumbLX,x.Gamepad.sThumbLY,x.Gamepad.sThumbRX,x.Gamepad.sThumbRY,x.Gamepad.bLeftTrigger,x.Gamepad.bRightTrigger,x.Gamepad.wButtons);last[slot]=x.dwPacketNumber;
                present[slot]=1;
            }
            else if(ret!=ERROR_SUCCESS&&present[slot]){fprintf(s,"%llu,%lu,0,0,0,0,0,0,0,0,0\n",(unsigned long long)now_us(),(unsigned long)slot);present[slot]=0;last[slot]=~0u;}
        }
        if(elapsed>=8000&&!sent_start)
        {
            XINPUT_VIBRATION v={50000,30000};for(DWORD slot=0;slot<4;slot++){DWORD ret=XInputSetState(slot,&v);fprintf(r,"%llu,%lu,start,%lu\n",(unsigned long long)now_us(),(unsigned long)slot,(unsigned long)ret);}fflush(r);sent_start=1;
        }
        if(elapsed>=8500&&!sent_stop)
        {
            XINPUT_VIBRATION v={0,0};for(DWORD slot=0;slot<4;slot++){DWORD ret=XInputSetState(slot,&v);fprintf(r,"%llu,%lu,stop,%lu\n",(unsigned long long)now_us(),(unsigned long)slot,(unsigned long)ret);}fflush(r);sent_stop=1;
        }
        Sleep(1);
    }
    fclose(s);fclose(r);return 0;
}
