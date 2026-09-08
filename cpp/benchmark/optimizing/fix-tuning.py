import json
import subprocess
from sys import exit
import optuna
import matplotlib.pyplot as plt

def objective(trial: optuna.Trial) -> float:
    params = {
        "T0": trial.suggest_float("T0", 2.0, 40.0),
        "alpha": trial.suggest_float("alpha", 0.99, 0.999999, log=True),
        "maxSteps": trial.suggest_int("maxSteps", 50_000, 1_000_000, step=25_000)
    }

    try:
        proc = subprocess.run(
            ["./build/algo_optimizing.exe", "--mode=fixed", "--params=" + json.dumps(params)],
            capture_output=True,
            text=True,
            timeout=30
        )
    except subprocess.TimeoutExpired:
        return 999999.0

    if proc.returncode != 0:
        return 999999.0

    try:
        result = json.loads(proc.stdout.strip())
    except json.JSONDecodeError:
        return 999999.0

    error_rate = result.get("error_rate", 1.0)
    average_ms = result.get("average_ms", 999999.0)

    if error_rate > 0:
        return 50000.0 * (1.0 + error_rate)

    return average_ms

def main() -> int:
    print("正在啟動 Optuna 貝氏優化器 (目標：尋找最速且 100% 收斂的 Fixed 參數)...")

    study = optuna.create_study(
        study_name="grid_shuffler_fixed_tuning",
        direction="minimize",
        sampler=optuna.samplers.TPESampler(seed=42)
    )

    study.optimize(objective, n_trials=50, show_progress_bar=True)

    print("\n" + "=" * 60)
    print("機器學習優化完成！")
    print(f"最短加權平均耗時: {study.best_value:.4f} ms")
    print("最佳超參數組合:")
    for key, value in study.best_params.items():
        if key == "alpha":
            print(f"   - {key}: {value:.8f}")
        else:
            print(f"   - {key}: {value}")
    print("=" * 60)

    try:
        optuna.visualization.matplotlib.plot_param_importances(study)
        plt.tight_layout()
        plt.savefig("hpo_param_importance.png", dpi=300)
        print("📊 參數重要性圖表已儲存至: hpo_param_importance.png")
    except Exception as e:
        print(f"Notice: 無法產出圖表 ({e})")

    return 0


if __name__ == '__main__':
    exit(main())