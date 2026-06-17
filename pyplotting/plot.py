#!/usr/bin/env python3
"""Robust plotting for the quantile benchmark.

Parses a benchmark log (the ``alg=... key=value ...`` lines printed by the C++
harness) into a pandas DataFrame, averages repeated runs, and renders error-bar
comparison charts for ingest time, query latency, space and accuracy.

Usage:
    python plot.py <benchmark_log.txt> [--outdir plots] [--show]

This is a leaner, less fragile replacement for the original main.py string
pipeline; it copes with any number of runs and any subset of algorithms.
"""
import argparse
import sys

import matplotlib
import numpy as np
import pandas as pd

QUANTILES = [0.5, 0.75, 0.9, 0.95, 0.99, 1.0]
ALG_LABELS = {"dd": "DDSketch", "sbeq": "Exact", "hist": "Histogram",
              "tdigest": "t-digest", "kll": "KLL"}


def parse_log(path):
    """Read result lines into a tidy DataFrame (one row per run/config)."""
    rows = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line.startswith("alg="):
                continue
            row = {}
            for tok in line.split():
                if "=" not in tok:
                    continue
                k, v = tok.split("=", 1)
                if k == "alg":
                    row[k] = v
                else:
                    try:
                        row[k] = float(v)
                    except ValueError:
                        row[k] = v
            rows.append(row)
    if not rows:
        raise SystemExit(f"no benchmark result lines (alg=...) found in {path}")
    return pd.DataFrame(rows)


def series_label(alg, threads):
    name = ALG_LABELS.get(alg, alg)
    if alg == "dd" and pd.notna(threads):
        return f"{name} t={int(threads)}"
    return name


def agg(df, metric):
    """Mean/min/max of `metric` grouped by (alg, num_threads, size)."""
    if metric not in df.columns:
        return None
    g = df.groupby(["alg", "num_threads", "size"], dropna=False)[metric]
    return g.agg(["mean", "min", "max"]).reset_index()


def plot_metric(df, metric, ylabel, title, outpath, show):
    import matplotlib.pyplot as plt

    a = agg(df, metric)
    if a is None or a["mean"].isna().all():
        print(f"  skip {metric}: not present in log")
        return
    plt.figure()
    for (alg, threads), grp in a.groupby(["alg", "num_threads"], dropna=False):
        grp = grp.sort_values("size")
        x = grp["size"].to_numpy()
        y = grp["mean"].to_numpy()
        if np.all(np.isnan(y)):
            continue
        yerr = [y - grp["min"].to_numpy(), grp["max"].to_numpy() - y]
        plt.errorbar(x, y, yerr=yerr, marker="o", capsize=3,
                     label=series_label(alg, threads))
    plt.xlabel("Data set [GB]")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend(loc="best")
    plt.grid(True, alpha=0.3)
    plt.savefig(outpath, dpi=120, bbox_inches="tight")
    print(f"  wrote {outpath}")
    if show:
        plt.show()
    plt.close()


def plot_accuracy(df, outpath, show):
    """Mean absolute relative error of each approximate alg vs the exact baseline."""
    import matplotlib.pyplot as plt

    qcols = [f"qv{q}" for q in QUANTILES if f"qv{q}" in df.columns]
    if "sbeq" not in df["alg"].values or not qcols:
        print("  skip accuracy: need exact (sbeq) runs and qv columns")
        return
    # exact reference per size (mean across runs)
    exact = df[df["alg"] == "sbeq"].groupby("size")[qcols].mean()
    plt.figure()
    for (alg, threads), grp in df.groupby(["alg", "num_threads"], dropna=False):
        if alg == "sbeq":
            continue
        per_size = grp.groupby("size")[qcols].mean()
        sizes, errs = [], []
        for size, qvals in per_size.iterrows():
            if size not in exact.index:
                continue
            ref = exact.loc[size]
            denom = ref.abs().clip(lower=1e-9)
            errs.append(float(((qvals - ref).abs() / denom).mean()))
            sizes.append(size)
        if sizes:
            order = np.argsort(sizes)
            plt.plot(np.array(sizes)[order], np.array(errs)[order],
                     marker="o", label=series_label(alg, threads))
    plt.xlabel("Data set [GB]")
    plt.ylabel("Mean abs. relative error vs exact")
    plt.title("Quantile accuracy vs exact baseline")
    plt.legend(loc="best")
    plt.grid(True, alpha=0.3)
    plt.savefig(outpath, dpi=120, bbox_inches="tight")
    print(f"  wrote {outpath}")
    if show:
        plt.show()
    plt.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("log", help="benchmark log file")
    ap.add_argument("--outdir", default=".", help="directory for the PNGs")
    ap.add_argument("--show", action="store_true", help="also display the figures")
    args = ap.parse_args()

    if not args.show:
        matplotlib.use("Agg")  # headless

    df = parse_log(args.log)
    print(f"parsed {len(df)} runs; algorithms: {sorted(df['alg'].unique())}")

    import os
    os.makedirs(args.outdir, exist_ok=True)
    out = lambda n: os.path.join(args.outdir, n)

    plot_metric(df, "t_add_s", "Ingest time [s]",
                "Ingest performance", out("ingest.png"), args.show)
    plot_metric(df, "t_get_qv_ms", "Query time [ms]",
                "Quantile query latency", out("query.png"), args.show)
    plot_metric(df, "space", "Space [# of buckets/values]",
                "Space requirements", out("space.png"), args.show)
    plot_accuracy(df, out("accuracy.png"), args.show)


if __name__ == "__main__":
    main()
