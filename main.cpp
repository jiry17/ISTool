#include <iostream>

#include "z3++.h"
#include "basic/config.h"
#include "basic/sygus/parser/parser.h"
#include "solver/component_based_solver/linear_encoder.h"
#include "solver/component_based_solver/component_based_solver.h"

int main() {
    std::string file = config::KSourcePath + "/max2.sl";
    std::cout << file << std::endl;
    auto spec = parser::getSyGuSSpecFromFile(file);
    auto* encoder = new LineEncoderBuilder();
    auto* cbs = new ComponentBasedSynthesizer(encoder);
    auto* solver = new CEGISSolver(cbs);
    auto res = solver->synthesis(spec);
    std::cout << res->toString() << std::endl;
    return 0;
}
