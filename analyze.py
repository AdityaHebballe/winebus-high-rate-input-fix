#!/usr/bin/env python3
import csv
import statistics
import sys

if len(sys.argv) != 3:
    raise SystemExit(f"usage: {sys.argv[0]} producer.csv xinput.csv")

with open(sys.argv[1], newline="") as f:
    phases = list(csv.DictReader(f))
with open(sys.argv[2], newline="") as f:
    samples = list(csv.DictReader(f))

samples = [{k: int(v) for k, v in row.items()} for row in samples]
delays = []
first_sample = samples[0]["realtime_us"] if samples else None
for phase in phases:
    if phase["event"] != "neutral":
        continue
    raw = int(phase["realtime_us"])
    trial = int(phase["trial"])
    if first_sample is None or raw < first_sample:
        print(f"trial {trial}: excluded (XInput probe was not ready at neutral edge)")
        continue
    # Require a zero state after the producer's neutral edge. All four generated
    # axes cannot center together during the rotating pattern. SDL's signed-axis
    # conversion maps physical center to either 0 or -1.
    zero = next((s for s in samples if s["realtime_us"] >= raw and
                 max(abs(s[a]) for a in ("lx", "ly", "rx", "ry")) <= 1), None)
    if zero:
        delay = (zero["realtime_us"] - raw) / 1000
        delays.append(delay)
        print(f"trial {trial}: XInput neutral delay {delay:.3f} ms")
    else:
        print(f"trial {trial}: no XInput neutral observed")

if delays:
    print(f"median={statistics.median(delays):.3f} ms max={max(delays):.3f} ms n={len(delays)}")
