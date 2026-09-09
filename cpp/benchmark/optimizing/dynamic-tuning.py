import json
import subprocess
from sys import exit
import optuna
import matplotlib.pyplot as plt

def objective(trial: optuna.Trial) -> float:
    params = {
        "min_step": trial.suggest_int("min_step", 10_000, 100_000, step=5_000),
        "size_mul": trial.suggest_int("size_mul", 50, 800, step=25),
        "T0": trial.suggest_float("T0", 5.0, 50.0),
        "alpha": trial.suggest_float("alpha", 0.99, 0.999999, log=True),
    }

    try:
        proc = subprocess.run(
            ["./build/algo_optimizing.exe", "--mode=dynamic", "--params=" + json.dumps(params)],
            capture_output=True,
            text=True,
            timeout=45
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
    print("🚀 正在啟動 Optuna 尋找【最佳動態自適應退火公式 (Dynamic)】...")

    study = optuna.create_study(
        study_name="grid_shuffler_dynamic_tuning",
        direction="minimize",
        sampler=optuna.samplers.TPESampler(seed=42)
    )

    study.optimize(objective, n_trials=100, show_progress_bar=True)

    print("\n" + "=" * 60)
    print("最佳動態自適應公式係數出爐！")
    print(f"全局最短平均耗時: {study.best_value:.4f} ms")
    print("最佳動態參數組合:")
    for key, value in study.best_params.items():
        if key == "Tend":
            print(f"   - {key}: {value:.6f}")
        elif key == "T0":
            print(f"   - {key}: {value:.4f}")
        else:
            print(f"   - {key}: {value}")
    print("=" * 60)

    try:
        optuna.visualization.matplotlib.plot_param_importances(study)
        plt.tight_layout()
        plt.savefig("hpo_dynamic_importance.png", dpi=300)
        print("📊 參數重要性圖表已儲存至: hpo_dynamic_importance.png")
    except Exception as e:
        print(f"Notice: 無法產出圖表 ({e})")

    return 0


if __name__ == '__main__':
    exit(main())