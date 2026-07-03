#!/usr/bin/env python3
import csv
import re
from pathlib import Path

import matplotlib.pyplot as plt

root = Path(__file__).resolve().parent
results = root / "results"
docs = root / "docs"
docs.mkdir(exist_ok=True)

def neutral_delays(name):
    producer = list(csv.DictReader((results / f"{name}-producer.csv").open()))
    states = [{k: int(v) for k, v in row.items()}
              for row in csv.DictReader((results / f"{name}-xinput.csv").open())]
    first = states[0]["realtime_us"]
    values = []
    for phase in producer:
        if phase["event"] != "neutral":
            continue
        edge = int(phase["realtime_us"])
        if edge < first:
            continue
        state = next((s for s in states if s["realtime_us"] >= edge and
                      max(abs(s[a]) for a in ("lx", "ly", "rx", "ry")) <= 1), None)
        if state:
            values.append((state["realtime_us"] - edge) / 1000)
    return values

baseline = neutral_delays("baseline")
patched = neutral_delays("patched")

labels = ["X", "Y", "D-pad right", "D-pad up", "Left trigger", "Right trigger"]
late = [531.137, 940.955, 1331.623, 1855.669, 2380.652, 2907.695]
patched_text = (results / "regression-patched-analysis.txt").read_text()
edge_delays = [float(v) for v in re.findall(r"PASS .*? ([0-9.]+) ms$", patched_text, re.MULTILINE)]
patched_max = max(edge_delays)

plt.style.use("seaborn-v0_8-whitegrid")
fig, axes = plt.subplots(1, 2, figsize=(12, 4.8))

axes[0].boxplot([baseline, patched], tick_labels=["Original", "Patched"], widths=.45,
                showmeans=True)
axes[0].scatter([1] * len(baseline), baseline, color="#c43b3b", zorder=3)
axes[0].scatter([2] * len(patched), patched, color="#25855a", zorder=3)
axes[0].set_yscale("log")
axes[0].set_ylabel("Raw neutral → XInput neutral (ms, log scale)")
axes[0].set_title("1 kHz four-axis queue latency")

axes[1].plot(labels, late, marker="o", linewidth=2.2, color="#c43b3b",
             label="Original press arrival")
axes[1].axhline(patched_max, color="#25855a", linewidth=2,
                label=f"Patched maximum ({patched_max:.3f} ms)")
axes[1].set_ylabel("Input delivery latency (ms)")
axes[1].set_title("Queue growth during two-controller flood")
axes[1].tick_params(axis="x", rotation=30)
axes[1].legend()

fig.suptitle("Wine SDL winebus high-report-rate input backlog", fontsize=14)
fig.tight_layout()
fig.savefig(docs / "latency-comparison.svg", bbox_inches="tight")
fig.savefig(docs / "latency-comparison.png", dpi=180, bbox_inches="tight")
print(f"wrote {docs / 'latency-comparison.svg'} and .png")
