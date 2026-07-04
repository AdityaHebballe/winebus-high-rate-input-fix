# SDL winebus high-rate input queue evidence

## Defect

The SDL winebus backend consumes one SDL event and returns one complete HID
input report per wait call. A controller reporting four changing axes at 1 kHz
therefore produces reports faster than Wine consumes them. Games receive old
axis and button states seconds after the physical events occurred.

The original issue was reproduced with a GameSir Nova 2 Lite (`3537:1098`) on
its 2.4 GHz receiver in Xbox mode. Linux evdev remained current while Proton
games continued moving for three to four seconds after both sticks stopped.

## Deterministic reproduction

`uinput_producer` creates an Xbox-compatible virtual controller and emits four
changing axes at 1 kHz. `xinput_probe.exe` timestamps each XInput state using
the same host real-time clock. `analyze.py` measures the raw-neutral to
XInput-neutral delay.

Environment:

- CachyOS Linux 7.1.2-3-cachyos, x86-64
- Proton-CachyOS 11.0-20260602, based on Valve Wine commit `9f8984e06928`
- SDL 2 compatibility layer 2.32.70

Results:

| Implementation | Median neutral delay | Maximum |
| --- | ---: | ---: |
| Original winebus | 3457.161 ms | 3462.164 ms |
| Coalescing patch | 1.099 ms | 1.339 ms |

The patched median is about 3145 times lower and matches the probe's roughly
1 ms polling resolution.

## Regression suite

`regression_producer` drives two simultaneous virtual Xbox controllers while
both send changing axes at 1 kHz. It injects ordered button and D-pad edges,
trigger transitions, hot-unplug/reconnect, and force feedback. The Windows
probe monitors four XInput slots and commands rumble on both active devices.

Patched results:

- Two distinct XInput slots identified correctly.
- All 14 ordered button and D-pad edges delivered.
- Maximum ordered-edge latency: 2.822 ms with the final refactored patch.
- Both trigger press/release transitions delivered.
- Hot-unplug and reconnect observed.
- Rumble accepted by XInput and received by both Linux uinput devices.

### Report-rate stress

The bounded SDL drain was tested from 125 Hz through 8 kHz overload:

| Rate | Median neutral delay | Maximum neutral delay |
| ---: | ---: | ---: |
| 125 Hz | 0.953 ms | 1.799 ms |
| 250 Hz | 1.292 ms | 2.151 ms |
| 500 Hz | 1.022 ms | 1.056 ms |
| 1 kHz | 0.925 ms | 0.959 ms |
| 2 kHz | 1.264 ms | 1.302 ms |
| 4 kHz | 1.367 ms | 1.839 ms |
| 8 kHz | 1.281 ms | 1.767 ms |

### Generic joystick path

An unmapped virtual joystick (`1234:abcd`) was read through WinMM to exercise
`SDL_JOYAXISMOTION` independently of SDL's GameController/XInput path. Button
press, button release, and neutral arrived in 1.698, 1.214, and 1.219 ms.

With the original backend, the injected press transitions arrived
progressively later as the queue grew:

| Transition | Delay |
| --- | ---: |
| X | 531.137 ms |
| Y | 940.955 ms |
| D-pad right | 1331.623 ms |
| D-pad up | 1855.669 ms |
| Left trigger | 2380.652 ms |
| Right trigger | 2907.695 ms |

## Safety properties

Only consecutive absolute-axis reports for the same device and report length
are coalesced. Button reports, hats, relative axes, device lifecycle events,
and force-feedback state reports remain ordering barriers. SDL draining is
limited to 256 queued events per pass to prevent a continuously producing
source from monopolizing the winebus thread.

## Build validation

On WineHQ master `685c5b6f`, complete SDL-enabled `winebus.so` builds and links
passed for both x86-64 and i386 with GCC 16 and Clang 22. The 32-bit results
were verified as ELF32 Intel 80386 shared objects. Wine's normal warning set
was enabled for every build, and `git diff --check` passed.

Raw CSV output and SHA-256 hashes are retained in `results/`.
