//
// Created by pro on 2022/2/13.
//

#include "istool/dsl/component/component_dataset.h"
#include "istool/ext/z3/z3_verifier.h"
#include "istool/selector/selector.h"
#include "istool/invoker/invoker.h"
#include "glog/logging.h"

int main(int argc, char** argv) {
    assert(argc == 3 || argc == 1);
    int benchmark_id;
    std::string output_name;
    if (argc == 4) {
        benchmark_id = std::atoi(argv[1]);
        output_name = argv[2];
    } else {
        benchmark_id = 1;
        output_name = "/tmp/629453237.out";
    }

    auto* spec = component::getTask(benchmark_id);
    auto* v = new Z3Verifier(dynamic_cast<Z3ExampleSpace*>(spec->example_space.get()));
    auto* guard = new TimeGuard(1e9);
    auto res = invoker::getExampleNum(spec, v, SolverToken::COMPONENT_BASED_SYNTHESIS, guard);

    LOG(INFO) << res.first << " " << res.second.toString();
    LOG(INFO) << guard->getPeriod();
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", res.first, res.second.toString());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}