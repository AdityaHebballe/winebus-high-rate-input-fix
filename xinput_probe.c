#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SAMPLES 200000

struct sample
{
    uint64_t realtime_us;
    DWORD packet;
    SHORT lx, ly, rx, ry;
    BYTE lt, rt;
    WORD buttons;
};

static uint64_t realtime_us(void)
{
    FILETIME ft;
    ULARGE_INTEGER value;
    GetSystemTimePreciseAsFileTime(&ft);
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return (value.QuadPart - 116444736000000000ULL) / 10;
}

int main(int argc, char **argv)
{
    const DWORD duration_ms = argc > 1 ? strtoul(argv[1], NULL, 10) : 40000;
    const char *output_path = argc > 2 ? argv[2] : "xinput.csv";
    struct sample *samples = calloc(MAX_SAMPLES, sizeof(*samples));
    DWORD start = GetTickCount(), last_packet = ~0u, count = 0;
    DWORD connected_polls = 0, disconnected_polls = 0;
    XINPUT_STATE state;
    FILE *output;
    DWORD ret;

    if (!samples) return 1;
    fprintf(stderr, "Polling XInput slot 0 for %lu ms...\n", (unsigned long)duration_ms);
    while (GetTickCount() - start < duration_ms)
    {
        ZeroMemory(&state, sizeof(state));
        ret = XInputGetState(0, &state);
        if (ret == ERROR_SUCCESS)
        {
            ++connected_polls;
            if (state.dwPacketNumber != last_packet && count < MAX_SAMPLES)
            {
                struct sample *s = &samples[count++];
                s->realtime_us = realtime_us();
                s->packet = state.dwPacketNumber;
                s->lx = state.Gamepad.sThumbLX; s->ly = state.Gamepad.sThumbLY;
                s->rx = state.Gamepad.sThumbRX; s->ry = state.Gamepad.sThumbRY;
                s->lt = state.Gamepad.bLeftTrigger; s->rt = state.Gamepad.bRightTrigger;
                s->buttons = state.Gamepad.wButtons;
                last_packet = state.dwPacketNumber;
            }
        }
        else ++disconnected_polls;
        Sleep(1);
    }
    if (!(output = fopen(output_path, "w"))) return 2;
    fprintf(output, "realtime_us,packet,lx,ly,rx,ry,lt,rt,buttons\n");
    for (DWORD i = 0; i < count; ++i)
        fprintf(output, "%llu,%lu,%d,%d,%d,%d,%u,%u,%u\n",
               (unsigned long long)samples[i].realtime_us, (unsigned long)samples[i].packet,
               samples[i].lx, samples[i].ly, samples[i].rx, samples[i].ry,
               samples[i].lt, samples[i].rt, samples[i].buttons);
    fclose(output);
    fprintf(stderr, "Captured %lu changed states; connected polls=%lu disconnected=%lu.\n",
            (unsigned long)count, (unsigned long)connected_polls, (unsigned long)disconnected_polls);
    free(samples);
    return 0;
}
