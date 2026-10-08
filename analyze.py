#!/usr/bin/env python3
"""Validate benchmark CSV, summarize repeated runs, and plot actual observations.
Usage: python3 analyze.py measured-results
Requires Python 3 and matplotlib. Uses the standard library for calculations.
"""
import csv
import math
import re
import statistics as stats
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

folder = Path(sys.argv[1] if len(sys.argv) > 1 else 'measured-results')
with (folder / 'raw.csv').open(newline='') as stream:
    rows = list(csv.DictReader(stream))
if not rows:
    raise SystemExit('No measurements found')
groups = defaultdict(list)
for row in rows:
    n = int(row['threads'])
    work = int(row['iterations_per_thread']) * n
    sec = float(row['elapsed_seconds'])
    rate = float(row['operations_per_second'])
    assert int(row['total_operations']) == int(row['checksum']) == work
    assert row['correct'] == '1' and sec > 0
    assert math.isclose(rate, work / sec, rel_tol=1e-9)
    assert math.isclose(float(row['ns_per_operation']), 1e9 / rate, rel_tol=1e-9)
    groups[(row['mode'], n)].append(row)

summary = []
for (mode, n), items in sorted(groups.items()):
    assert len({r['repeat'] for r in items}) == len(items)
    rates = [float(r['operations_per_second']) for r in items]
    times = [float(r['elapsed_seconds']) for r in items]
    quartiles = stats.quantiles(rates, n=4, method='inclusive') if len(rates) > 1 else [rates[0]]*3
    summary.append(dict(mode=mode, threads=n, samples=len(items),
        median_seconds=stats.median(times), min_seconds=min(times), max_seconds=max(times),
        median_ops_per_sec=stats.median(rates), min_ops_per_sec=min(rates), max_ops_per_sec=max(rates),
        q1_ops_per_sec=quartiles[0], q3_ops_per_sec=quartiles[2],
        active_cache_lines=int(items[0]['active_cache_lines']),
        runs_with_cgroup_throttling=sum(int(r['cgroup_throttled_usec_delta']) > 0 for r in items)))
with (folder / 'summary.csv').open('w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=summary[0].keys())
    writer.writeheader(); writer.writerows(summary)

counts = sorted({int(r['threads']) for r in rows})
colors = {'packed':'#cf623b', 'padded':'#167a72', 'true_shared':'#62718d'}
labels = {'packed':'Packed: separate counters, shared lines',
          'padded':'Padded: one counter per line',
          'true_shared':'True sharing: one common counter'}
plt.rcParams.update({'font.family':'DejaVu Sans', 'font.size':10,
    'axes.spines.top':False, 'axes.spines.right':False, 'axes.labelcolor':'#243246',
    'text.color':'#243246', 'axes.edgecolor':'#bdc7d2'})
fig, axes = plt.subplots(1, 2, figsize=(13, 5.8), gridspec_kw={'width_ratios':[1.8,1]})
fig.subplots_adjust(top=.79, bottom=.22, left=.075, right=.97, wspace=.27)
fig.suptitle('False sharing: measured throughput and padding benefit', x=.075,
             ha='left', fontsize=18, fontweight='bold', y=.97)
sample_counts = sorted({len(v) for v in groups.values()})
repeat_text = ', '.join(map(str, sample_counts))
fig.text(.075, .91, f"{len(rows)} measured runs | {repeat_text} repeats per case | atomic increments, relaxed ordering", fontsize=11)
for mode in ['padded', 'packed', 'true_shared']:
    series = [r for r in summary if r['mode'] == mode]
    if not series: continue
    xs = [r['threads'] for r in series]
    ys = [r['median_ops_per_sec']/1e6 for r in series]
    lo = [(r['median_ops_per_sec']-r['min_ops_per_sec'])/1e6 for r in series]
    hi = [(r['max_ops_per_sec']-r['median_ops_per_sec'])/1e6 for r in series]
    axes[0].errorbar(xs, ys, yerr=[lo,hi], label=labels[mode], color=colors[mode],
                     marker='o', linewidth=2, capsize=4, markersize=5)
axes[0].set_ylabel('Aggregate throughput (million operations / second)')
axes[0].set_ylim(bottom=0)
axes[0].legend(loc='upper left', bbox_to_anchor=(0,1.16), fontsize=8.5, frameon=False)
for n in counts:
    packed = {r['repeat']:float(r['operations_per_second']) for r in groups.get(('packed',n),[])}
    padded = {r['repeat']:float(r['operations_per_second']) for r in groups.get(('padded',n),[])}
    common = sorted(set(packed) & set(padded))
    if not common: continue
    ratios = [padded[r]/packed[r] for r in common]
    median = stats.median(ratios)
    axes[1].errorbar(n, median, yerr=[[median-min(ratios)],[max(ratios)-median]],
                     color=colors['padded'], marker='o', capsize=4, linewidth=1.7)
    axes[1].annotate(f'{median:.2f}x', (n,median), xytext=(0,8),
                     textcoords='offset points', ha='center', fontsize=9)
axes[1].axhline(1, color='#a0a9b5', linestyle='--', linewidth=1)
axes[1].set_ylim(bottom=0)
axes[1].set_ylabel('Padded / packed throughput (same-repeat ratio)')
axes[1].set_title('Median padding benefit', fontsize=11, pad=12)
for ax in axes:
    ax.set_xlabel('Worker threads')
    ax.set_xticks(counts)
    ax.grid(axis='y', color='#e4e9ef', linewidth=.7)
    ax.set_axisbelow(True)
fig.text(.075,.095,'Points: medians. Whiskers: observed minimum to maximum, not confidence intervals.', fontsize=10)
environment_path = folder / 'environment.txt'
environment = environment_path.read_text() if environment_path.exists() else ''
quota = re.search(r'cgroup cpu.max:\s*\n(\d+)\s+(\d+)', environment)
quota_note = f" CPU quota: {int(quota[1])/int(quota[2]):g} equivalents." if quota else ''
max_lines = max(int(r['active_cache_lines']) for r in rows if r['mode'] == 'packed') if any(r['mode']=='packed' for r in rows) else 0
footnote = f"Scheduling and topology affect scaling.{quota_note} Packed case at {max(counts)} threads uses {max_lines} cache line(s)."
fig.text(.075,.052,footnote, fontsize=10)
fig.savefig(folder/'false-sharing-graph.png', dpi=190, facecolor='white')
plt.close(fig)
print(f'Validated {len(rows)} measurements; wrote summary.csv and false-sharing-graph.png')
for r in summary:
    print(f"{r['mode']:12s} {r['threads']:2d} threads  median {r['median_seconds']:.6f}s  "
          f"{r['median_ops_per_sec']/1e6:.3f} Mops/s  "
          f"range {r['min_ops_per_sec']/1e6:.3f}-{r['max_ops_per_sec']/1e6:.3f}")
