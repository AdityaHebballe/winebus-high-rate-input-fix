#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t now_us(void){FILETIME f;ULARGE_INTEGER u;GetSystemTimePreciseAsFileTime(&f);u.LowPart=f.dwLowDateTime;u.HighPart=f.dwHighDateTime;return(u.QuadPart-116444736000000000ULL)/10;}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:"winmm.csv";DWORD duration=argc>2?strtoul(argv[2],0,10):20000,start=GetTickCount();
    JOYINFOEX last[16]={{0}};BOOL valid[16]={0};UINT count=joyGetNumDevs();FILE *out=fopen(path,"w");
    if(!out)return 2;
    fprintf(out,"realtime_us,id,x,y,z,r,u,v,buttons,pov\n");
    while(GetTickCount()-start<duration)
    {
        for(UINT id=0;id<count&&id<16;id++)
        {
            JOYINFOEX j={.dwSize=sizeof(j),.dwFlags=JOY_RETURNALL};
            if(joyGetPosEx(id,&j)==JOYERR_NOERROR&&(!valid[id]||memcmp(&last[id],&j,sizeof(j))))
            {fprintf(out,"%llu,%u,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",(unsigned long long)now_us(),id,j.dwXpos,j.dwYpos,j.dwZpos,j.dwRpos,j.dwUpos,j.dwVpos,j.dwButtons,j.dwPOV);last[id]=j;valid[id]=1;}
        }
        Sleep(1);
    }
    fclose(out);return 0;
}
