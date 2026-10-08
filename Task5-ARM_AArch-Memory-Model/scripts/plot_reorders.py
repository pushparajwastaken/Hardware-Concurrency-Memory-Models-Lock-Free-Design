import numpy as np
import matplotlib.pyplot as plt

# ---------- Graph 1: cumulative reorders vs runs ----------
runs, cum = np.loadtxt("reorders_vs_runs.txt", comments="#", unpack=True)

fig1, ax1 = plt.subplots(figsize=(9, 5))
ax1.plot(runs, cum, color="tab:blue", lw=1.8)
ax1.set_xlabel("Runs (iterations)")
ax1.set_ylabel("Cumulative reorders detected")
ax1.set_title("Memory reordering (r1 == 0 && r2 == 0) vs runs")
ax1.grid(alpha=0.3)
ax1.ticklabel_format(style="plain", axis="x")
fig1.tight_layout()
fig1.savefig("reorders_vs_runs.png", dpi=150)

# ---------- Graph 2: reorders per second ----------
sec, reorders, runs_in_sec = np.loadtxt("reorders_per_second.txt", comments="#", unpack=True)

# The last second is partial (program ends mid-second), so drop it from the
# raw plot if there is more than one second of data.
if len(sec) > 1:
    sec_f, re_f, ru_f = sec[:-1], reorders[:-1], runs_in_sec[:-1]
else:
    sec_f, re_f, ru_f = sec, reorders, runs_in_sec

fig2, ax2 = plt.subplots(figsize=(9, 5))
ax2.bar(sec_f, re_f, color="tab:orange", width=0.7)
ax2.axhline(re_f.mean(), color="k", ls="--", lw=1, label=f"mean = {re_f.mean():.1f}/s")
ax2.set_xlabel("Second of program runtime")
ax2.set_ylabel("Reorders observed in that second")
ax2.set_title("Reorders per second")
ax2.set_xticks(sec_f)
ax2.legend()
ax2.grid(alpha=0.3, axis="y")
fig2.tight_layout()
fig2.savefig("reorders_per_second.png", dpi=150)

# Summary
print(f"Total runs: {int(runs[-1])}, total reorders: {int(cum[-1])}, "
      f"reorder rate: {cum[-1] / runs[-1] * 100:.4f}% of runs")
plt.show()