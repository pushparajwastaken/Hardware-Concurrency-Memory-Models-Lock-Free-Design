import pandas as pd, matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt, numpy as np

df = pd.read_csv("bench_results.csv")
C = {"MESI": "#4C78A8", "MOESI": "#F58518"}
wl_order = ["producer_consumer","migratory","read_mostly","write_contention","private_data","mixed_random"]
wl_lbl = ["producer/\nconsumer","migratory","read\nmostly","write\ncontention","private\ndata","mixed\nrandom"]
base = df[df.workload.isin(wl_order)].copy()

def grouped(ax, col, ylabel, title, fmt=None, scale=1.0):
    x = np.arange(len(wl_order)); w = 0.38
    for i, p in enumerate(["MESI", "MOESI"]):
        v = [base[(base.workload==k)&(base.protocol==p)][col].iloc[0]*scale for k in wl_order]
        b = ax.bar(x+(i-0.5)*w, v, w, label=p, color=C[p])
        if fmt: ax.bar_label(b, labels=[fmt(t) for t in v], fontsize=7, padding=2)
    ax.set_xticks(x); ax.set_xticklabels(wl_lbl, fontsize=8)
    ax.set_ylabel(ylabel); ax.set_title(title, fontsize=11, fontweight="bold"); ax.legend(fontsize=8)
    ax.spines[["top","right"]].set_visible(False); ax.grid(axis="y", alpha=.3)

fig, axs = plt.subplots(2, 3, figsize=(18, 10))
fig.suptitle("MESI vs MOESI -- coherence simulator benchmarks (8 cores, 200k ops/workload, all invariants I1-I5 held)",
             fontsize=14, fontweight="bold")

# 1 memory writes per 1k ops
grouped(axs[0,0], "memWrites", "memory writes per 1,000 ops", "Memory write traffic",
        fmt=lambda t: f"{t:.1f}", scale=1000/200000)

# 2 cache-to-cache transfers per 1k ops
grouped(axs[0,1], "cacheToCache", "cache-to-cache transfers per 1,000 ops",
        "Cache-to-cache data supplies", fmt=lambda t: f"{t:.0f}", scale=1000/200000)

# 3 hit rate
base["hitrate"] = base.hits/base.ops*100
grouped(axs[0,2], "hitrate", "hit rate (%)", "Cache hit rate (identical for both)", fmt=lambda t: f"{t:.0f}%")
axs[0,2].set_ylim(0, 112)

# 4 bus transaction mix (identical in both protocols -> plot once)
ax = axs[1,0]; m = base[base.protocol=="MESI"].set_index("workload").loc[wl_order]
x = np.arange(len(wl_order)); bottom = np.zeros(len(wl_order))
for col, lab, colr in [("busRd","BusRd","#54A24B"),("busRdX","BusRdX","#E45756"),("busUpgr","BusUpgr","#B279A2")]:
    v = m[col].values/m.ops.values*100
    ax.bar(x, v, 0.6, bottom=bottom, label=lab, color=colr); bottom += v
ax.set_xticks(x); ax.set_xticklabels(wl_lbl, fontsize=8); ax.set_ylabel("bus transactions per 100 ops")
ax.set_title("Bus traffic mix (MESI = MOESI)", fontsize=11, fontweight="bold")
ax.legend(fontsize=8); ax.spines[["top","right"]].set_visible(False); ax.grid(axis="y", alpha=.3)

# 5 scaling
ax = axs[1,1]
for wl, ls, mk in [("scale_producer_consumer","-","o"),("scale_mixed_random","--","s")]:
    for p in ["MESI","MOESI"]:
        d = df[(df.workload==wl)&(df.protocol==p)].sort_values("cores")
        ax.plot(d.cores, d.memWrites/d.ops*1000, ls, marker=mk, color=C[p],
                label=f"{p} - {wl.replace('scale_','').replace('_','/')}")
ax.set_xscale("log", base=2); ax.set_xticks([2,4,8,16,32]); ax.set_xticklabels([2,4,8,16,32])
ax.set_xlabel("cores"); ax.set_ylabel("memory writes per 1,000 ops")
ax.set_title("Core-count scaling of memory writes", fontsize=11, fontweight="bold")
ax.legend(fontsize=7); ax.spines[["top","right"]].set_visible(False); ax.grid(alpha=.3)

# 6 throughput
ax = axs[1,2]
tr = df[df.workload.isin(["throughput_raw","throughput_checked"])]
x = np.arange(2); w = 0.38
for i, p in enumerate(["MESI","MOESI"]):
    v = [tr[(tr.workload==k)&(tr.protocol==p)].hits.iloc[0]/1e6 for k in ["throughput_raw","throughput_checked"]]
    b = ax.bar(x+(i-0.5)*w, v, w, label=p, color=C[p]); ax.bar_label(b, fmt="%.2f M", fontsize=8, padding=2)
ax.set_xticks(x); ax.set_xticklabels(["simulator only","simulator + invariant checker"])
ax.set_ylabel("million ops / second"); ax.set_title("Simulation throughput (8 cores, 2M ops)", fontsize=11, fontweight="bold")
ax.legend(fontsize=8); ax.spines[["top","right"]].set_visible(False); ax.grid(axis="y", alpha=.3)

plt.tight_layout(rect=[0,0,1,0.96])
plt.savefig("benchmark_charts.png", dpi=130)
print("saved")
