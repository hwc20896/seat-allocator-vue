#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""比較 自動 / 預設Config / 調參後 三種 annealing 設定的執行效率

將同一份 Google Benchmark JSON 中的 6 個 family 依共同前綴歸組：
    BM_Shuffle4Automatic / BM_Shuffle4DefaultConfig / BM_Shuffle4TunedConfig
    BM_Shuffle8Automatic / BM_Shuffle8DefaultConfig / BM_Shuffle8TunedConfig

每個尺寸 n 比較：
  real_time  總耗時（ms）          越小越快
  AvgSteps   平均收斂步數          越小收斂越快
  ErrorRate  失敗率                越小越好
  us/step    每步成本（演算法純效率，理論上三者應接近）

用法：
    python benchmark-compare.py curve.json [--out compare.png] [--show]
    python benchmark-compare.py --run \
        --bin build-release/seat_allocator_vue_algo_benchmark.exe \
        --out curve.json --show
"""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

plt.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei",
                                   "Noto Sans CJK SC", "WenQuanYi Micro Hei",
                                   "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

SUFFIXES = {"Automatic": "自動", "DefaultConfig": "預設", "TunedConfig": "調參"}
STYLES = {"Automatic": "o-", "DefaultConfig": "s--", "TunedConfig": "^:"}


def parse_range(name: str) -> int:
    """從 name 取出 range 值，例如 'BM_X/8/min_time:0.2/repeats:5_mean' -> 8"""
    m = re.search(r"/(\d+)/min_time", name)
    if not m:
        raise ValueError(f"無法從 name 解析 range: {name}")
    return int(m.group(1))


def load_rows(path: Path) -> list[dict]:
    data = json.loads(path.read_text(encoding="utf-8"))
    ctx = data.get("context", {})
    print(f"[{path.name}] {ctx.get('date', '?')} | {ctx.get('host_name', '?')}"
          f" | {ctx.get('num_cpus', '?')} 核 | {ctx.get('library_version', '?')}")
    return data["benchmarks"]


def group_families(rows: list[dict]) -> dict[str, dict[str, dict[int, dict]]]:
    """base名 -> {後綴 -> {n: mean 聚合列}}（只取 aggregate_name == 'mean'）"""
    fams: dict[str, dict[int, dict]] = {}
    for r in rows:
        if r.get("aggregate_name") != "mean":
            continue
        name = r["name"].split("/")[0]
        fams.setdefault(name, {})[parse_range(r["name"])] = r

    groups: dict[str, dict[str, dict[int, dict]]] = {}
    for name, pts in fams.items():
        for suf in SUFFIXES:
            if name.endswith(suf):
                groups.setdefault(name[: -len(suf)], {})[suf] = pts
                break
    for pts in groups.values():
        for suf in pts:
            pts[suf] = dict(sorted(pts[suf].items()))
    return groups


def short_name(base: str) -> str:
    s = base.replace("BM_Shuffle", "")
    return f"{s}鄰居" if s else base


def fmt(x: float, spec: str) -> str:
    return f"{x:{spec}}" if np.isfinite(x) else "-"


def compare_file(path: Path, out_png: Path, show: bool) -> None:
    groups = group_families(load_rows(path))
    if not groups:
        sys.exit(f"{path.name}: 找不到可歸組的 family（後綴需為 {list(SUFFIXES)}）")

    fig, axes = plt.subplots(2, 2, figsize=(13, 9))

    for i, (base, variants) in enumerate(groups.items()):
        color = f"C{i}"
        ns = sorted(set().union(*(set(pts) for pts in variants.values())))
        series = {}
        for suf in SUFFIXES:  # 固定順序：自動、預設、調參
            pts = variants.get(suf)
            if pts is None:
                continue
            t = np.array([pts[n]["real_time"] for n in ns]) / 1e6          # ms
            s = np.array([pts[n].get("AvgSteps", np.nan) for n in ns])
            e = np.array([pts[n].get("ErrorRate", np.nan) for n in ns]) * 100
            u = np.array([pts[n].get("AlgoTimeUS", np.nan) for n in ns])
            series[suf] = dict(t=t, s=s, e=e,
                               u=u / np.where(s > 0, s, np.nan))           # us/步
        if len(series) < 2:
            print(f"!! {base}: 有效 family 不足，跳過")
            continue

        suf_list = list(series)
        tag = short_name(base)
        print(f"== {tag}（{base}） ==")
        header = "   n  " + "  ".join(f"{SUFFIXES[s]:>7}ms" for s in suf_list)
        header += "  " + "  ".join(f"{SUFFIXES[s]:>8}步" for s in suf_list)
        header += "  " + "  ".join(f"{SUFFIXES[s]:>6}err%" for s in suf_list)
        print(header)
        for k, n in enumerate(ns):
            row = f"  {n:>3}  " + "  ".join(fmt(series[s]["t"][k], "7.3f") for s in suf_list)
            row += "  " + "  ".join(fmt(series[s]["s"][k], "8.0f") for s in suf_list)
            row += "  " + "  ".join(fmt(series[s]["e"][k], "6.1f") for s in suf_list)
            print(row)

        base_suf = suf_list[0]  # 自動為基準
        for suf in suf_list[1:]:
            sp = series[base_suf]["t"] / series[suf]["t"]
            geo = float(np.exp(np.nanmean(np.log(sp))))
            n_ok = (np.isfinite(series[base_suf]["s"]) & (series[base_suf]["s"] > 0)
                    & np.isfinite(series[suf]["s"]) & (series[suf]["s"] > 0))
            st = (float(np.mean(series[suf]["s"][n_ok] / series[base_suf]["s"][n_ok])) - 1) * 100
            us = (float(np.mean(series[suf]["u"][n_ok] / series[base_suf]["u"][n_ok])) - 1) * 100
            print(f"  → {SUFFIXES[suf]} vs {SUFFIXES[base_suf]}: 加速比 {geo:.2f}x"
                  f"，步數 {st:+.1f}%，每步成本 {us:+.1f}%"
                  f"，錯誤率 {np.nanmean(series[base_suf]['e']):.1f}%"
                  f" → {np.nanmean(series[suf]['e']):.1f}%")

        for suf in suf_list:
            style = STYLES[suf]
            axes[0, 0].plot(ns, series[suf]["t"], style, color=color,
                            label=f"{tag} {SUFFIXES[suf]}")
            if suf != base_suf:
                axes[0, 1].plot(ns, series[base_suf]["t"] / series[suf]["t"],
                                style, color=color, label=f"{tag} {SUFFIXES[suf]}")
            axes[1, 0].plot(ns, series[suf]["s"], style, color=color,
                            label=f"{tag} {SUFFIXES[suf]}")
            axes[1, 1].plot(ns, series[suf]["e"], style, color=color,
                            label=f"{tag} {SUFFIXES[suf]}")

    axes[0, 0].set(xlabel="grid size n", ylabel="mean time (ms)",
                   xscale="log", yscale="log", title="Time: 三種設定")
    axes[0, 1].axhline(1.0, color="gray", ls=":", lw=1)
    axes[0, 1].set(xlabel="grid size n", ylabel="speedup (vs 自動)",
                   title="Speedup（>1 = 比自動快）")
    axes[1, 0].set(xlabel="grid size n", ylabel="avg steps",
                   title="Steps: 三種設定")
    axes[1, 1].set(xlabel="grid size n", ylabel="error rate (%)",
                   title="Error rate: 三種設定")
    for ax in axes.flat:
        ax.legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(out_png, dpi=150)
    print(f"圖已存檔: {out_png}")
    if show:
        plt.show()


def run_benchmark(bin_path: Path, json_path: Path) -> None:
    if not bin_path.exists():
        sys.exit(f"找不到執行檔: {bin_path}（請先 cmake --build build-release）")
    json_path.parent.mkdir(parents=True, exist_ok=True)
    cmd = [str(bin_path), f"--benchmark_out={json_path}", "--benchmark_out_format=json"]
    print("執行:", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main() -> None:
    ap = argparse.ArgumentParser(description="比較 自動/預設/調參 三種 annealing config 的執行效率")
    ap.add_argument("json", nargs="*", help="benchmark JSON 檔（可多份，各別出圖）")
    ap.add_argument("--run", action="store_true",
                    help="先執行 benchmark 產生 JSON（輸出到第一個 json 參數）")
    ap.add_argument("--bin", default="build-release/seat_allocator_vue_algo_benchmark.exe")
    ap.add_argument("--out", help="輸出 PNG 路徑（預設為 JSON 同名 .compare.png）")
    ap.add_argument("--show", action="store_true")
    args = ap.parse_args()

    if args.run:
        if not args.json:
            args.json = ["curve.json"]
        run_benchmark(Path(args.bin), Path(args.json[0]))

    if not args.json:
        ap.error("需要至少一個 JSON 檔，或加上 --run")

    for jp in args.json:
        p = Path(jp)
        out = Path(args.out) if args.out else p.with_name(p.stem + ".compare.png")
        compare_file(p, out, args.show)


if __name__ == "__main__":
    main()