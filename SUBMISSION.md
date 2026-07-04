# Proposed Wine submission

## Title

`winebus: Coalesce pending SDL axis input reports.`

## Rationale

The SDL backend currently converts every SDL axis event into a complete HID
input report, while `sdl_bus_wait()` consumes one SDL event and returns one
queued report per call. Controllers reporting several changing axes at 1 kHz
can therefore enqueue reports faster than Wine consumes them. XInput clients
receive increasingly stale controller states even though Linux evdev remains
current.

The patch drains at most 256 pending SDL events per pass and coalesces only
consecutive absolute-axis reports for the same device and report length.
Buttons, hats, relative axes, device lifecycle events, and force-feedback
state remain non-coalescible ordering barriers.

## Evidence

The deterministic uinput/XInput test measured:

| Implementation | Median neutral delay | Maximum neutral delay |
| --- | ---: | ---: |
| Original | 3457.161 ms | 3462.164 ms |
| Patched | 1.099 ms | 1.339 ms |

The final patched regression run used two simultaneous 1 kHz virtual
controllers. All button, D-pad, and trigger transitions arrived, with maximum
ordered-edge latency of 2.822 ms. Hot-unplug/reconnect, distinct XInput slots,
and force feedback to both devices also passed.

Bounded-drain stress from 125 Hz through 8 kHz kept median neutral latency
between 0.925 and 1.367 ms. A separate unmapped generic SDL joystick tested
through WinMM delivered its button edges and neutral within 1.698 ms.

Full source, methodology, plots, analyzer output, and raw captures:

https://github.com/AdityaHebballe/winebus-high-rate-input-fix

## Build validation

- WineHQ master at `685c5b6f`.
- Native SDL-enabled `dlls/winebus.sys/winebus.so` builds and links passed
  with GCC 16 and Clang 22 on both x86-64 and i386.
- Both 32-bit results were verified as ELF32 Intel i386 shared objects.
- `git diff --check` passed.

GCC build artifact SHA-256 values:

- 64-bit: `f8432d292aece511accdc9f5a74f6bed8cd9dec0927210df4f7e054710a23535`
- 32-bit: `a687005355f4df53a357c46b68fe23a0b072a8d9dfd8b9b0b8c5df600a290358`

Clang build artifact SHA-256 values:

- 64-bit: `d8be1430c51028fe1cec3f5fa905eab8f45f47a929eff12bac1386f1bf7f7094`
- 32-bit: `0a8a531253df352d3677575d735479d66bd05812378c4de2096d5664364c251f`
