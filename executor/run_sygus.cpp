//
// Created by pro on 2022/2/15.
//

#include "istool/selector/split/splitor.h"
#include "istool/selector/split/finite_splitor.h"
#include "istool/selector/split/z3_splitor.h"
#include "istool/selector/split/split_selector.h"
#include "istool/invoker/invoker.h"
#include "istool/sygus/sygus.h"
#include "istool/sygus/parser/parser.h"
#include "istool/ext/z3/z3_example_space.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/dsl/samplesy/samplesy_dsl.h"
#include "istool/ext/vsa/vsa_extension.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include <ctime>

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    std::string benchmark_name, output_name, solver_name;
    if (argc == 4) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
    } else {
        solver_name = "vsa";
        benchmark_name = " /tmp/tmp.wHOuYKwdWN/tests/string/dr-name-long.sl";
        output_name = "/tmp/629453237.out";
    }
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    auto* v = sygus::getVerifier(spec);
    spec->env->random_engine.seed(time(0));
    auto solver_token = invoker::string2TheoryToken(solver_name);
    auto* guard = new TimeGuard(1e9);
    auto result = invoker::getExampleNum(spec, v, solver_token, guard);
    std::cout << result.first << " " << result.second.toString() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}