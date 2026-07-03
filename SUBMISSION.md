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
- Native 64-bit `dlls/winebus.sys/winebus.so` build passed.
- Changed `bus_sdl.c` and `unixlib.c` passed forced 32-bit compilation with
  Wine's warnings promoted to errors.
- `git diff --check` passed.

The host did not have a linkable 32-bit SDL library, so a complete 32-bit
`winebus.so` link was not claimed.
