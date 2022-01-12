from show import *
from config import RunnerConfig
from show import draw_trend
from executor import get_all_benchmark, execute


if __name__ == "__main__":
    runner = RunnerConfig("/tmp/tmp.hABJQGVQ89/cmake-build-debug-remote-host/run",
                          time_limit=100, memory_limit=4, flags=["polygen"], name="polygen")
    random_runner = RunnerConfig("/tmp/tmp.hABJQGVQ89/cmake-build-debug-remote-host/run",
                                 time_limit=100, memory_limit=4, flags=["polygen_random"], name="polygen_random")
    benchmark_list = get_all_benchmark("/tmp/tmp.hABJQGVQ89/tests/polygen/")
    result = execute(runner, benchmark_list, "/tmp/tmp.hABJQGVQ89/runner/cache/test_clia.json")
    result = execute(random_runner, benchmark_list, "/tmp/tmp.hABJQGVQ89/runner/cache/test_clia.json")
    lis_tasks(result["polygen_random"])
    val = CaredValue(lambda x: x["time"], "time")
    draw_trend(result, val, "1.png")
