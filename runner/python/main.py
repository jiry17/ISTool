from show import *
from config import RunnerConfig
from show import draw_trend
from executor import get_all_benchmark, execute

val_time = CaredValue(lambda x: x["time"], "time")
val_example = CaredValue(lambda x: int(x["result"][0].split(' ')[0]), "example", draw_type=DrawType.LINEAR, merge_type=MergeType.AVE)
src_path = "/tmp/tmp.i5X31IAVA3/"

def run_clia():
    solver_list = ["polygen", "eusolver"]
    verifier_list = ["default", "random", "splitor50", "splitor200"]
    for solver in solver_list:
        runner_list = [RunnerConfig(src_path + "cmake-build-debug-yifan/executor/run_sygus",
                                    time_limit=100, memory_limit=4, flags=[solver, verifier], name=verifier)
                    for verifier in verifier_list]
        benchmark_list = get_all_benchmark(src_path + "tests/polygen/")

        cache_name = solver + "_clia"

        for runner in runner_list:
            result = execute(runner, benchmark_list, src_path + "runner/cache/" + cache_name + ".json")
        draw_trend(get_all_solved(result), val_example, cache_name + "_example.png")

def run_string():
    solver_list = ["maxflash", "vsa"]
    verifier_list = ["default", "random", "splitor50", "splitor200"]
    for solver in solver_list:
        runner_list = [RunnerConfig(src_path + "cmake-build-debug-yifan/executor/run_sygus",
                                    time_limit=100, memory_limit=4, flags=[solver, verifier], name=verifier)
                    for verifier in verifier_list]
        benchmark_list = get_all_benchmark(src_path + "tests/string/")

        cache_name = solver + "_string"

        for runner in runner_list:
            result = execute(runner, benchmark_list, src_path + "runner/cache/" + cache_name + ".json")
        print("solver", solver)
        list_all_result(get_all_solved(result), val_example)
        draw_trend(get_all_solved(result), val_example, cache_name + "_example.png")
        

def run_samplesy():
    solver_list = ["splitor", "randomsy", "samplesy"]
    runner_list = [RunnerConfig(src_path + "cmake-build-debug-yifan/run",
                                time_limit=1200, memory_limit=8, flags=[solver], name=solver)
                   for solver in solver_list]
    benchmark_list = get_all_benchmark(src_path + "tests/string/")

    for runner in runner_list:
        result = execute(runner, benchmark_list, src_path + "runner/cache/test_samplesy.json")
    draw_trend(result, val_example, "samplesy_example.png")

def run_bv():
    solver_list = ["default", "random", "prior", "splitor50", "splitor200"]
    runner_list = [RunnerConfig(src_path + "cmake-build-debug-yifan/executor/run_cbs_icse10",
                                time_limit=1200, memory_limit=8, flags=[solver], name=solver)
                   for solver in solver_list]
    benchmark_list = [str(i) for i in range(1, 22)]

    for runner in runner_list:
        result = execute(runner, benchmark_list, src_path +  "runner/cache/test_bv.json")
    draw_trend(get_all_solved(result), val_example, "bv_example.png")



if __name__ == "__main__":
    run_bv()
    run_clia()
    run_string()