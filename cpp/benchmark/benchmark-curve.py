#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""座位分配演算法基準測試曲線分析工具

讀取 Google Benchmark 的 JSON 輸出（需含 aggregate 行），對每個 benchmark family：
  1. 擬合單冪律  t ≈ c·n^k（log-log 迴歸）
  2. 若 R² < 0.9（曲線呈 U 型），再加擬雙項模型 t ≈ c1·n^a + c2·n^(-b)
  3. 時間與 AvgSteps 各做一次，並輸出曲線圖 PNG

用法：
    python benchmark-curve.py [json 路徑] [png 路徑] [--show]
"""

import json
import re
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

plt.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei",
                                   "Noto Sans CJK SC", "WenQuanYi Micro Hei",
                                   "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

def parse_range(name: str) -> int:
    """從 name 取出 range 值，例如 'BM_X/8/min_time:..._mean' -> 8"""
    m = re.search(r"/(\d+)/min_time", name)
    if m:
        return int(m.group(1))
    raise ValueError(f"無法從 name 解析 range: {name}")


def load_family(rows: list[dict], family_index: int, field: str) -> tuple[np.ndarray, np.ndarray]:
    """取某 family 的 mean 聚合行，回傳按 n 排序的 (n, values)"""
    pts = [r for r in rows
           if r["family_index"] == family_index and r.get("aggregate_name") == "mean"]
    if len(pts) < 2:
        raise ValueError(f"family {family_index} 的 mean 聚合行不足")
    pts.sort(key=lambda r: parse_range(r["name"]))
    n = np.array([parse_range(r["name"]) for r in pts], dtype=float)
    v = np.array([r[field] for r in pts], dtype=float)
    return n, v


def fit_power(n: np.ndarray, t: np.ndarray) -> tuple[float, float, float, np.ndarray]:
    """單冪律 t ≈ c·n^k"""
    k, log_c = np.polyfit(np.log(n), np.log(t), 1)
    c = float(np.exp(log_c))
    t_fit = c * n ** k
    r2 = 1 - np.sum((t - t_fit) ** 2) / np.sum((t - t.mean()) ** 2)
    return k, c, r2, t_fit


def fit_two_term(n: np.ndarray, t: np.ndarray) -> tuple[float, float, float, float, float, np.ndarray, float]:
    """雙項模型 t ≈ c1·n^a + c2·n^(-b)；a/b 網格搜尋，c1/c2 最小平方解"""
    best = None
    for a in np.linspace(0.5, 3.0, 60):
        for b in np.linspace(0.5, 12.0, 120):
            A = np.vstack([n ** a, n ** (-b)]).T
            c, *_ = np.linalg.lstsq(A, t, rcond=None)
            if c[0] <= 0 or c[1] <= 0:
                continue
            r2 = 1 - np.sum((t - A @ c) ** 2) / np.sum((t - t.mean()) ** 2)
            if best is None or r2 > best[0]:
                best = (r2, float(a), float(b), float(c[0]), float(c[1]))
    if best is None:
        raise ValueError("雙項模型擬合失敗（找不到正係數解）")
    r2, a, b, c1, c2 = best
    t_fit = c1 * n ** a + c2 * n ** (-b)
    n_star = (c2 * b / (c1 * a)) ** (1 / (a + b))
    return a, b, c1, c2, r2, t_fit, n_star


def family_name(rows: list[dict], family_index: int) -> str:
    for r in rows:
        if r["family_index"] == family_index:
            return r["name"].split("/")[0]
    return f"family {family_index}"


def analyze(name: str, n: np.ndarray, v: np.ndarray, unit: str) -> tuple[np.ndarray, np.ndarray, np.ndarray, str]:
    """擬合並印出結果，回傳 (n, 原始值, 擬合曲線, 圖例名稱)"""
    k, c, r2, fit1 = fit_power(n, v)
    print(f"  單冪律  {name} ≈ {c:.3g}·n^{k:.2f} {unit}   R² = {r2:.3f}")
    if r2 < 0.9:
        a, b, c1, c2, r2b, fit2, n_star = fit_two_term(n, v)
        print(f"  雙項    {name} ≈ {c1:.3g}·n^{a:.2f} + {c2:.3g}·n^(-{b:.2f}) {unit}"
              f"   R² = {r2b:.3f}   最低點 n ≈ {n_star:.0f}")
        return n, v, fit2, f"{name} 雙項 R²={r2b:.2f}"
    return n, v, fit1, f"{name} R²={r2:.2f}"


def main() -> None:
    json_path = sys.argv[1] if len(sys.argv) > 1 else "curve.json"
    png_path = sys.argv[2] if len(sys.argv) > 2 else "curve.png"
    show = "--show" in sys.argv

    data = json.loads(Path(json_path).read_text(encoding="utf-8"))
    rows = data["benchmarks"]
    ctx = data.get("context", {})

    print("量測環境:", ctx.get("date", "?"), "|", ctx.get("host_name", "?"),
          f"| {ctx.get('num_cpus', '?')} 核 |", ctx.get("library_version", "?"))

    families = sorted({r["family_index"] for r in rows})
    if not families:
        sys.exit("JSON 中沒有 benchmark 資料")

    fig, axes = plt.subplots(1, 2, figsize=(12, 5))

    for fi in families:
        fname = family_name(rows, fi)
        print(f"== {fname} ==")

        n, t = load_family(rows, fi, "real_time")
        res_t = analyze("t", n, t, "ms")
        line, = axes[0].plot(res_t[0], res_t[1], "o", label=res_t[3])
        axes[0].plot(res_t[0], res_t[2], "--", color=line.get_color())

        try:
            n_s, s = load_family(rows, fi, "AvgSteps")
        except KeyError:
            continue
        res_s = analyze("steps", n_s, s, "步")
        line, = axes[1].plot(res_s[0], res_s[1], "o", label=res_s[3])
        axes[1].plot(res_s[0], res_s[2], "--", color=line.get_color())

    axes[0].set(xlabel="grid size n", ylabel="mean time (ms)",
                xscale="log", yscale="log", title="Time curve")
    axes[1].set(xlabel="grid size n", ylabel="avg steps",
                xscale="log", title="Steps curve")
    for ax in axes:
        ax.legend()

    fig.tight_layout()
    fig.savefig(png_path, dpi=150)
    print(f"圖已存檔: {png_path}")
    if show:
        plt.show()


if __name__ == "__main__":
    main()