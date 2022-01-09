from show import *
from executor import *


if __name__ == "__main__":
    runner = RunnerConfig("/tmp/tmp.hABJQGVQ89/cmake-build-debug-remote-host/run",
                          time_limit=1, memory_limit=4, flags=["vsa"])
    benchmark_list = get_all_benchmark("/tmp/tmp.hABJQGVQ89/tests/string/")
    execute(runner, benchmark_list, "/tmp/tmp.hABJQGVQ89/runner/cache.json")
