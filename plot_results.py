#!/usr/bin/env python3

import csv
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


if len(sys.argv) > 1:
    input_file = Path(sys.argv[1])
else:
    input_file = Path("metrics_log.txt")

if not input_file.exists():
    print(f"ERROR: File not found: {input_file}")
    sys.exit(1)


# ---------------------------------------------------------
# Read CSV
# ---------------------------------------------------------

timestamps = []
commit_counts = []
identity_counts = []
account_counts = []
info_counts = []
buffer_pct = []
cpu_pct = []

with input_file.open("r", encoding="utf-8") as f:
    reader = csv.DictReader(f)

    for row in reader:
        try:
            sec = int(row["Seconds"])
            nsec = int(row["Nanoseconds"])

            commit = int(row["Commit_Count"])
            identity = int(row["Identity_Count"])
            account = int(row["Account_Count"])
            info = int(row["Info_Count"])

            buffer = float(row["Buffer_Occupancy_Pct"])
            cpu = float(row["CPU_Pct"])

        except (ValueError, KeyError):
            continue

        timestamps.append(sec + nsec / 1_000_000_000.0)

        commit_counts.append(commit)
        identity_counts.append(identity)
        account_counts.append(account)
        info_counts.append(info)

        buffer_pct.append(buffer)
        cpu_pct.append(cpu)


if len(timestamps) < 2:
    print("ERROR: Not enough samples.")
    sys.exit(1)


# ---------------------------------------------------------
# Convert to numpy arrays
# ---------------------------------------------------------

timestamps = np.array(timestamps)

commit_counts = np.array(commit_counts)
identity_counts = np.array(identity_counts)
account_counts = np.array(account_counts)
info_counts = np.array(info_counts)

buffer_pct = np.array(buffer_pct)
cpu_pct = np.array(cpu_pct)



time_hours = (timestamps - timestamps[0]) / 3600.0


# ---------------------------------------------------------
# Messages per monitor interval
# ---------------------------------------------------------

messages_per_interval = (
    commit_counts
    + identity_counts
    + account_counts
    + info_counts
)


# ---------------------------------------------------------
# Actual time between samples
# ---------------------------------------------------------

dt = np.diff(timestamps)


# ---------------------------------------------------------
# Jitter,  ideal period = 1 second
# ---------------------------------------------------------

jitter_ms = (dt - 1.0) * 1000.0

jitter_time_hours = time_hours[1:]


# ---------------------------------------------------------
# Actual message rate in Hz
# ---------------------------------------------------------

message_rate_hz = messages_per_interval[1:] / dt

rate_time_hours = time_hours[1:]

buffer_for_rate = buffer_pct[1:]
cpu_for_rate = cpu_pct[1:]



output_dir = Path("plots")
output_dir.mkdir(exist_ok=True)


# =========================================================
# 1. JITTER PLOT
# =========================================================

plt.figure(figsize=(12, 5))

plt.plot(
    jitter_time_hours,
    jitter_ms,
    linewidth=0.6,
    color="tab:blue"
)

plt.axhline(
    0,
    linestyle="--",
    linewidth=0.8,
    color="black"
)

plt.xlabel("Time from start (hours)")
plt.ylabel("Jitter (ms)")
plt.title("Periodic Monitor Thread Jitter")

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    output_dir / "jitter_plot.png",
    dpi=180
)

plt.close()


# =========================================================
# 2. MESSAGE RATE + BUFFER OCCUPANCY
# =========================================================

fig, ax1 = plt.subplots(figsize=(12, 5))

line1 = ax1.plot(
    rate_time_hours,
    message_rate_hz,
    color="tab:blue",
    linewidth=0.6,
    label="Incoming message rate (Hz)"
)

ax1.set_xlabel("Time from start (hours)")
ax1.set_ylabel(
    "Incoming message rate (Hz)",
    color="tab:blue"
)

ax1.tick_params(
    axis="y",
    labelcolor="tab:blue"
)

ax1.grid(True, alpha=0.3)


ax2 = ax1.twinx()

line2 = ax2.plot(
    rate_time_hours,
    buffer_for_rate,
    color="tab:green",
    linewidth=0.9,
    linestyle="--",
    label="Buffer occupancy (%)"
)

ax2.set_ylabel(
    "Circular buffer occupancy (%)",
    color="tab:green"
)

ax2.tick_params(
    axis="y",
    labelcolor="tab:green"
)

# Προσαρμοσμένη κλίμακα ώστε να φαίνεται καθαρά ο buffer
ax2.set_ylim(
    0,
    max(25, np.max(buffer_for_rate) * 1.1)
)


lines = line1 + line2

labels = [
    line.get_label()
    for line in lines
]

ax1.legend(
    lines,
    labels,
    loc="upper right"
)

plt.title(
    "Incoming Message Load and Circular Buffer Occupancy"
)

fig.tight_layout()

plt.savefig(
    output_dir / "load_buffer_plot.png",
    dpi=180
)

plt.close()


# =========================================================
# 3. CPU USAGE vs MESSAGE RATE
# =========================================================

correlation = np.corrcoef(
    message_rate_hz,
    cpu_for_rate
)[0, 1]


plt.figure(figsize=(8, 6))

plt.scatter(
    message_rate_hz,
    cpu_for_rate,
    s=8,
    alpha=0.25,
    color="tab:blue"
)

plt.xlabel("Incoming message rate (Hz)")
plt.ylabel("CPU busy (%)")

plt.title(
    f"CPU Usage vs Incoming Message Rate "
    f"(r = {correlation:.3f})"
)

plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig(
    output_dir / "cpu_plot.png",
    dpi=180
)

plt.close()


# ---------------------------------------------------------
# Statistics
# ---------------------------------------------------------

duration_hours = (
    timestamps[-1] - timestamps[0]
) / 3600.0

total_messages = int(
    np.sum(messages_per_interval)
)

mean_rate = np.mean(message_rate_hz)
max_rate = np.max(message_rate_hz)

mean_cpu = np.mean(cpu_pct)
max_cpu = np.max(cpu_pct)

mean_buffer = np.mean(buffer_pct)
max_buffer = np.max(buffer_pct)

mean_abs_jitter = np.mean(
    np.abs(jitter_ms)
)

max_abs_jitter = np.max(
    np.abs(jitter_ms)
)


# ---------------------------------------------------------
# Terminal output
# ---------------------------------------------------------

print("----------------------------------------")
print("ANALYSIS COMPLETE")
print("----------------------------------------")

print(f"Input file       : {input_file}")
print(f"Samples          : {len(timestamps)}")
print(f"Duration         : {duration_hours:.3f} h")

print(f"Total messages   : {total_messages}")
print(f"Mean msg rate    : {mean_rate:.2f} Hz")
print(f"Max msg rate     : {max_rate:.2f} Hz")

print(f"Mean CPU         : {mean_cpu:.2f} %")
print(f"Max CPU          : {max_cpu:.2f} %")

print(f"Mean buffer      : {mean_buffer:.2f} %")
print(f"Max buffer       : {max_buffer:.2f} %")

print(f"Mean |jitter|    : {mean_abs_jitter:.4f} ms")
print(f"Max |jitter|     : {max_abs_jitter:.4f} ms")

print(f"CPU-rate corr r  : {correlation:.4f}")

print("----------------------------------------")
print("Created:")
print("  plots/jitter_plot.png")
print("  plots/load_buffer_plot.png")
print("  plots/cpu_plot.png")
print("----------------------------------------")
