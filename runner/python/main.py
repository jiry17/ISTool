from show import *
from config import RunnerConfig
from show import draw_trend
from executor import get_all_benchmark, execute

val_time = CaredValue(lambda x: x["time"], "time")
val_example = CaredValue(lambda x: int(x["result"][0].split(' ')[0]), "example", draw_type=DrawType.LINEAR, merge_type=MergeType.AVE)

def run_clia():
    solver_list = ["polygen", "prand", "pselect", "pselect0.1"]
    runner_list = [RunnerConfig("/tmp/tmp.hABJQGVQ89/cmake-build-debug-remote-host/run",
                                time_limit=100, memory_limit=4, flags=[solver], name=solver)
                   for solver in solver_list]
    benchmark_list = get_all_benchmark("/tmp/tmp.hABJQGVQ89/tests/polygen/")

    for runner in runner_list:
        result = execute(runner, benchmark_list, "/tmp/tmp.hABJQGVQ89/runner/cache/test_clia.json")
    for solver in solver_list:
        compare("polygen", solver, [val_example], result)
    draw_trend(result, val_time, "clia_time.png")
    draw_trend(result, val_example, "clia_example.png")

def run_string():
    solver_list = ["maxflash", "mselect"]
    runner_list = [RunnerConfig("/tmp/tmp.hABJQGVQ89/cmake-build-debug-remote-host/run",
                                time_limit=300, memory_limit=4, flags=[solver], name=solver)
                   for solver in solver_list]
    benchmark_list = get_all_benchmark("/tmp/tmp.hABJQGVQ89/tests/string/")

    for runner in runner_list:
        result = execute(runner, benchmark_list, "/tmp/tmp.hABJQGVQ89/runner/cache/test_string.json")
    compare("maxflash", "mselect", [val_example], result)
    lis_tasks(result["maxflash"])
    draw_trend(result, val_time, "string_time.png")
    draw_trend(result, val_example, "string_example.png")


if __name__ == "__main__":
    run_string()
