import json
import subprocess
from sys import exit
import optuna
import matplotlib.pyplot as plt

def objective(trial: optuna.Trial) -> float:
    one_minus_alpha = trial.suggest_float("1_minus_alpha", 0.0005, 0.010, log=True)
    alpha = 1.0 - one_minus_alpha

    trial.set_user_attr("actual_alpha", alpha)

    params = {
        "min_step": 5000,
        "size_mul": 300,
        "alpha": alpha
    }

    try:
        proc = subprocess.run(
            ["./build/algo_optimizing.exe", "--mode=dynamic", "--params=" + json.dumps(params)],
            capture_output=True, 
            text=True,
            timeout=30
        )
    except subprocess.TimeoutExpired:
        return 99999.0

    if proc.returncode != 0:
        return 99999.0

    try:
        result = json.loads(proc.stdout.strip())
    except json.JSONDecodeError:
        return 99999.0

    if result.get("error_rate", 1.0) > 0:
        return 50000.0 * (1.0 + result["error_rate"])

    return result.get("average_ms", 99999.0)

def main() -> int:
    print("正在啟動 Optuna 1D 極限尋谷實驗（探索純粹的 alpha 物理極限）...")

    study = optuna.create_study(
        study_name="alpha_pure_tuning",
        direction="minimize",
        sampler=optuna.samplers.TPESampler(seed=42)
    )
    
    study.optimize(objective, n_trials=300, show_progress_bar=True)

    best_alpha = study.best_trial.user_attrs["actual_alpha"]
    print("\n" + "=" * 60)
    print(f"最低耗時: {study.best_value:.4f} ms")
    print(f"唯一真理 alpha: {best_alpha:.16f}")
    print("=" * 60)

    try:
        optuna.visualization.matplotlib.plot_slice(study, params=["1_minus_alpha"])
        plt.title("The 1D Loss Basin of Alpha (1 - Alpha)", fontsize=13, weight="bold")
        plt.xlabel("1 - Alpha (Log Scale)", fontsize=11)
        plt.ylabel("Execution Time (ms)", fontsize=11)
        plt.tight_layout()
        plt.savefig("alpha_1d_basin.png", dpi=300)
        print("1D 物理峽谷圖已儲存至: alpha_1d_basin.png")
    except Exception as e:
        print(f"Notice: ({e})")

    return 0


if __name__ == '__main__':
    exit(main())