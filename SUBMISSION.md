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

Full source, methodology, plots, analyzer output, and raw captures:

https://github.com/AdityaHebballe/winebus-high-rate-input-fix

## Build validation

- WineHQ master at `1bb0da91a5e737323656403199aa3a3b2a9ffd9c`.
- Native 64-bit `dlls/winebus.sys/winebus.so` build and link passed.
- Native 32-bit SDL-enabled `dlls/winebus.sys/winebus.so` build and link
  passed. The result is an ELF32 Intel i386 shared object.
- `git diff --check` passed.

Build artifact SHA-256 values:

- 64-bit: `f8432d292aece511accdc9f5a74f6bed8cd9dec0927210df4f7e054710a23535`
- 32-bit: `a687005355f4df53a357c46b68fe23a0b072a8d9dfd8b9b0b8c5df600a290358`
