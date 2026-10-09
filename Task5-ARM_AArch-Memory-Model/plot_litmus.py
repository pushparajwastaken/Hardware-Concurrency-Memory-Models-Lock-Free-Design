#!/usr/bin/env python3
"""
plot_litmus.py - plot arm-litmus results from several CPU pairs on the same graphs.

Usage:
    python plot_litmus.py [results_dir] [--out plots] [--show]

Expected layout (written by arm-litmus.cpp):
    results/summary.txt
    results/<cpu pair>/<test>/reorders_vs_runs.txt
    results/<cpu pair>/<test>/reorders_per_second.txt

Figures written to the --out folder (default ./plots):
    SB_cumulative.png  MP_cumulative.png  LB_cumulative.png   cumulative weak outcomes vs runs
    SB_per_second.png  MP_per_second.png  LB_per_second.png   weak outcomes per second
    rate_by_instruction.png                                    weak-outcome % for every variant
    outcome_mix.png                                            share of each (r0,r1) outcome
In every line/bar chart each CPU pair has its own colour, so pairs are compared directly.
"""
import argparse
import os
import re
import sys
import warnings

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.ticker import FuncFormatter
from matplotlib.patches import Patch

# name = folder name written by arm-litmus.cpp, label, instructions
FAMILIES = {
    "SB": {
        "title": "SB (store buffering)   weak outcome: r0 = 0 and r1 = 0",
        "weak": (0, 0),
        "variants": [
            ("SB_relaxed",         "relaxed",          "str; ldr"),
            ("SB_release_acquire", "release/acquire",  "stlr; ldar"),
            ("SB_seq_cst",         "seq_cst",          "stlr; ldar"),
            ("SB_relaxed_dmb_ish", "relaxed + dmb ish", "str; dmb ish; ldr"),
        ],
    },
    "MP": {
        "title": "MP (message passing)   weak outcome: flag seen but data stale (r0 = 1, r1 = 0)",
        "weak": (1, 0),
        "variants": [
            ("MP_relaxed",             "relaxed",             "str; str  /  ldr; ldr"),
            ("MP_release_acquire",     "release/acquire",     "str; stlr  /  ldar; ldr"),
            ("MP_relaxed_fences",      "relaxed + fences",    "str; dmb; str  /  ldr; dmb ishld; ldr"),
            ("MP_only_writer_release", "writer release only", "str; stlr  /  ldr; ldr"),
        ],
    },
    "LB": {
        "title": "LB (load buffering)   weak outcome: r0 = 1 and r1 = 1",
        "weak": (1, 1),
        "variants": [
            ("LB_relaxed",       "relaxed",       "ldr; str"),
            ("LB_store_release", "store-release", "ldr; stlr"),
        ],
    },
}
ALL_TESTS = [v[0] for f in FAMILIES.values() for v in f["variants"]]


# ----------------------------------------------------------------- loading
def natural_key(s):
    return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", s)]


def pretty_pair(name):
    m = re.fullmatch(r"cpu(\d+)-(\d+)", name)
    return f"CPUs {m.group(1)} & {m.group(2)}" if m else name


def load_table(path):
    if not os.path.isfile(path):
        return None
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        try:
            a = np.loadtxt(path, comments="#", ndmin=2)
        except Exception:
            return None
    return a if a.size else None


def load_runs(root, pair, test):
    a = load_table(os.path.join(root, pair, test, "reorders_vs_runs.txt"))
    if a is None or a.shape[1] < 2:
        return None
    return a[:, 0], a[:, 1]


def load_seconds(root, pair, test):
    a = load_table(os.path.join(root, pair, test, "reorders_per_second.txt"))
    if a is None or a.shape[1] < 3:
        return None
    sec, weak, runs = a[:, 0], a[:, 1], a[:, 2]
    if len(sec) > 1:                     # last second is partial, drop it (as in plot_reorders.py)
        sec, weak, runs = sec[:-1], weak[:-1], runs[:-1]
    return sec, weak


def load_summary(root):
    """returns {(pair, test): dict(iterations, weak, percent, runtime, c=(c00,c01,c10,c11))}"""
    path = os.path.join(root, "summary.txt")
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path) as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            p = line.split()
            if len(p) < 10:
                continue
            out[(p[0], p[1])] = dict(
                iterations=int(p[2]), weak=int(p[3]), percent=float(p[4]), runtime=float(p[5]),
                c=tuple(int(x) for x in p[6:10]),
            )
    return out


def discover_pairs(root):
    pairs = []
    for name in os.listdir(root):
        d = os.path.join(root, name)
        if os.path.isdir(d) and any(os.path.isdir(os.path.join(d, t)) for t in ALL_TESTS):
            pairs.append(name)
    return sorted(pairs, key=natural_key)


# ----------------------------------------------------------------- figures
def pair_legend(fig, pairs, colors, y=0.95):
    handles = [Line2D([0], [0], color=colors[p], lw=3, label=pretty_pair(p)) for p in pairs]
    fig.legend(handles=handles, loc="upper center", bbox_to_anchor=(0.5, y), ncol=len(pairs), frameon=False)


def family_figure(fam_key, kind, root, pairs, colors, outdir):
    fam = FAMILIES[fam_key]
    variants = fam["variants"]
    ncols = 2
    nrows = (len(variants) + 1) // 2
    fig, axes = plt.subplots(nrows, ncols, figsize=(12, 4.3 * nrows), sharey=True, squeeze=False)
    flat = axes.ravel()
    gmax = 0.0

    for ax, (slug, name, instr) in zip(flat, variants):
        peak = 0.0
        for pair in pairs:
            if kind == "cumulative":
                d = load_runs(root, pair, slug)
                if d is None:
                    continue
                ax.plot(d[0], d[1], color=colors[pair], lw=2, label=pretty_pair(pair))
            else:
                d = load_seconds(root, pair, slug)
                if d is None:
                    continue
                ax.plot(d[0], d[1], color=colors[pair], lw=1.6, marker="o", ms=4, label=pretty_pair(pair))
            peak = max(peak, float(np.max(d[1])))
        gmax = max(gmax, peak)
        ax.set_title(f"{name}\n{instr}", fontsize=10)
        ax.grid(alpha=0.3)
        if peak == 0:
            ax.text(0.5, 0.5, "no weak outcomes observed", transform=ax.transAxes,
                    ha="center", va="center", color="gray", fontsize=11)
        if kind == "cumulative":
            ax.xaxis.set_major_formatter(FuncFormatter(
                lambda v, _: f"{v / 1e6:g}M" if v >= 1e6 else (f"{v / 1e3:g}k" if v > 0 else "0")))
            ax.set_xlabel("Runs (iterations)")
        else:
            ax.set_xlabel("Second of test runtime")
            ax.xaxis.get_major_locator().set_params(integer=True)
    for ax in flat[len(variants):]:
        ax.set_visible(False)
    for r in range(nrows):
        axes[r][0].set_ylabel("Cumulative weak outcomes" if kind == "cumulative"
                              else "Weak outcomes in that second")
    flat[0].set_ylim(0, gmax * 1.08 if gmax > 0 else 1)

    what = "cumulative weak outcomes vs runs" if kind == "cumulative" else "weak outcomes per second"
    fh = fig.get_figheight()
    # fig.text instead of suptitle: tight_layout would reserve extra space for a suptitle
    fig.text(0.5, 1 - 0.08 / fh, f"{fam['title']}\n{what}", ha="center", va="top", fontsize=12)
    pair_legend(fig, pairs, colors, y=1 - 0.68 / fh)
    fig.tight_layout(rect=[0, 0, 1, 1 - 1.0 / fh])
    path = os.path.join(outdir, f"{fam_key}_{'cumulative' if kind == 'cumulative' else 'per_second'}.png")
    fig.savefig(path, dpi=150)
    return path


def layout_positions():
    """x positions of every variant on a shared axis, with a gap between families."""
    centers, spans, pos = [], [], 0.0
    for fk, fam in FAMILIES.items():
        start = pos
        for _ in fam["variants"]:
            centers.append(pos)
            pos += 1
        spans.append((fk, start - 0.5, pos - 0.5))
        pos += 0.6
    return centers, spans


def rate_figure(pairs, colors, summary, outdir):
    centers, spans = layout_positions()
    tests = ALL_TESTS
    fig, ax = plt.subplots(figsize=(15, 6))
    w = 0.8 / len(pairs)
    ymax = 0.0
    for j, pair in enumerate(pairs):
        xs, hs = [], []
        for c, t in zip(centers, tests):
            s = summary.get((pair, t))
            xs.append(c - 0.4 + w * (j + 0.5))
            hs.append(s["percent"] if s else 0.0)
        ax.bar(xs, hs, width=w * 0.92, color=colors[pair], label=pretty_pair(pair))
        for x, h in zip(xs, hs):
            ax.text(x, h + 0.2, f"{h:.2f}%" if h > 0 else "0", ha="center", va="bottom",
                    rotation=90, fontsize=7, color="black" if h > 0 else "gray")
        ymax = max(ymax, max(hs))
    ymax = ymax if ymax > 0 else 1
    ax.set_ylim(0, ymax * 1.3)
    for i, (fk, a, b) in enumerate(spans):
        if i % 2 == 0:
            ax.axvspan(a, b, color="0.95", zorder=0)
        ax.text((a + b) / 2, ymax * 1.24, fk, ha="center", va="top", fontsize=11, fontweight="bold")
    ax.set_xticks(centers)
    ax.set_xticklabels([f"{v[1]}\n{v[2].split('  /  ')[0]}" for fam in FAMILIES.values() for v in fam["variants"]],
                       rotation=25, ha="right", fontsize=8)
    ax.set_ylabel("Weak outcomes (% of runs)")
    ax.set_title("How often the weak (reordered) outcome appears, by instruction variant and CPU pair")
    ax.grid(alpha=0.3, axis="y")
    ax.legend(frameon=False, loc="upper right", bbox_to_anchor=(1.0, 0.93))
    fig.tight_layout()
    path = os.path.join(outdir, "rate_by_instruction.png")
    fig.savefig(path, dpi=150)
    return path


def mix_figure(pairs, colors, summary, outdir):
    centers, spans = layout_positions()
    cell_colors = ["tab:blue", "tab:green", "tab:orange", "tab:purple"]      # (0,0) (0,1) (1,0) (1,1)
    cell_names = ["(0,0)", "(0,1)", "(1,0)", "(1,1)"]
    weak_cell = {}
    for fk, fam in FAMILIES.items():
        wc = fam["weak"][0] * 2 + fam["weak"][1]
        for v in fam["variants"]:
            weak_cell[v[0]] = wc

    fig, axes = plt.subplots(len(pairs), 1, figsize=(15, 3.6 * len(pairs)), sharex=True, squeeze=False)
    for ax, pair in zip(axes.ravel(), pairs):
        for x, t in zip(centers, ALL_TESTS):
            s = summary.get((pair, t))
            if not s:
                continue
            total = max(sum(s["c"]), 1)
            bottom = 0.0
            for k in range(4):
                frac = s["c"][k] / total
                if frac <= 0:
                    continue                      # no bar (a zero-height hatched bar would draw a stray line)
                is_weak = (k == weak_cell[t])
                ax.bar(x, frac, bottom=bottom, width=0.8, color=cell_colors[k],
                       hatch="///" if is_weak else None, edgecolor="red" if is_weak else "white",
                       linewidth=1.2 if is_weak else 0.5)
                bottom += frac
        for i, (fk, a, b) in enumerate(spans):
            if i % 2 == 0:
                ax.axvspan(a, b, color="0.95", zorder=0)
        ax.set_ylim(0, 1)
        ax.set_ylabel(f"{pretty_pair(pair)}\nshare of runs")
    centers_labels = [f"{v[1]}" for fam in FAMILIES.values() for v in fam["variants"]]
    axes[-1][0].set_xticks(centers)
    axes[-1][0].set_xticklabels(centers_labels, rotation=25, ha="right", fontsize=8)
    for fk, a, b in spans:
        axes[0][0].text((a + b) / 2, 1.04, fk, ha="center", va="bottom", fontsize=11, fontweight="bold")
    handles = [Patch(facecolor=cell_colors[k], label=f"(r0,r1) = {cell_names[k]}") for k in range(4)]
    handles.append(Patch(facecolor="white", edgecolor="red", hatch="///", label="weak outcome"))
    fig.legend(handles=handles, loc="lower center", ncol=5, frameon=False)
    fig.suptitle("Outcome mix per variant (hatched red segment = weak outcome)", fontsize=12)
    fig.tight_layout(rect=[0, 0.04, 1, 0.96])
    path = os.path.join(outdir, "outcome_mix.png")
    fig.savefig(path, dpi=150)
    return path


def print_table(pairs, summary):
    print("\nWeak outcomes (% of runs)")
    header = f"{'variant':<28}" + "".join(f"{pretty_pair(p):>16}" for p in pairs)
    print(header)
    print("-" * len(header))
    for fk, fam in FAMILIES.items():
        for slug, name, _ in fam["variants"]:
            row = f"{fk + ' ' + name:<28}"
            for p in pairs:
                s = summary.get((p, slug))
                row += f"{s['percent']:>15.4f}%" if s else f"{'-':>16}"
            print(row)


# ----------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description="Plot arm-litmus results for several CPU pairs.")
    ap.add_argument("results_dir", nargs="?", default="litmus_results")
    ap.add_argument("--out", default="plots", help="folder for the PNG files (default: plots)")
    ap.add_argument("--show", action="store_true", help="also open the figures in a window")
    args = ap.parse_args()

    if not os.path.isdir(args.results_dir):
        sys.exit(f"Results folder '{args.results_dir}' not found. Run ./litmus first.")
    pairs = discover_pairs(args.results_dir)
    if not pairs:
        sys.exit(f"No <cpu pair>/<test>/ folders found in '{args.results_dir}'.")
    os.makedirs(args.out, exist_ok=True)

    cmap = plt.get_cmap("tab10")
    colors = {p: cmap(i % 10) for i, p in enumerate(pairs)}
    summary = load_summary(args.results_dir)

    written = []
    for fk in FAMILIES:
        written.append(family_figure(fk, "cumulative", args.results_dir, pairs, colors, args.out))
    for fk in FAMILIES:
        written.append(family_figure(fk, "per_second", args.results_dir, pairs, colors, args.out))
    if summary:
        written.append(rate_figure(pairs, colors, summary, args.out))
        written.append(mix_figure(pairs, colors, summary, args.out))
        print_table(pairs, summary)
    else:
        print("summary.txt missing or empty: skipping rate_by_instruction.png and outcome_mix.png")

    print("\nWrote:")
    for p in written:
        print("  " + p)
    if args.show:
        plt.show()
    else:
        plt.close("all")


if __name__ == "__main__":
    main()