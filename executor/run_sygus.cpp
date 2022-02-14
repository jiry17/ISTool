//
// Created by pro on 2022/2/15.
//

#include "istool/invoker/invoker.h"
#include "istool/sygus/sygus.h"
#include "istool/sygus/parser/parser.h"

int main(int argc, char** argv) {
    assert(argc == 4 || argc == 1);
    std::string benchmark_name, output_name, solver_name;
    if (argc == 4) {
        benchmark_name = argv[1];
        output_name = argv[2];
        solver_name = argv[3];
    } else {
        solver_name = "eusolver";
        benchmark_name = "/tmp/tmp.i5X31IAVA3/tests/polygen/sum.sl";
        output_name = "/tmp/629453237.out";
    }
    auto *spec = parser::getSyGuSSpecFromFile(benchmark_name);
    auto solver_token = invoker::string2TheoryToken(solver_name);
    auto* v = sygus::getVerifier(spec);
    auto* guard = new TimeGuard(1e9);
    auto result = invoker::getExampleNum(spec, v, solver_token, guard);
    std::cout << result.first << " " << result.second.toString() << std::endl;
    FILE* f = fopen(output_name.c_str(), "w");
    fprintf(f, "%d %s\n", result.first, result.second.toString().c_str());
    fprintf(f, "%.10lf\n", guard->getPeriod());
}