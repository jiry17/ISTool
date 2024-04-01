//
// Created by pro on 2022/9/18.
//

#include "istool/basic/config.h"
#include "istool/basic/grammar.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/language/incre_program.h"
#include "istool/incre/language/incre_syntax.h"
#include "istool/incre/language/incre_dp.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/grammar/incre_grammar_semantics.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/trans/incre_trans.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;
#define Print(x) std::cout << #x " = " << x << std::endl;

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = ::config::KIncreParserPath + "/benchmarks/01knapsack.f";
    }
    IncreProgram prog = io::parseFromF(path);
    printProgram(prog);

    auto env = std::make_shared<Env>();
    incre::config::applyConfig(prog.get(), env.get());

    auto ctx = buildContext(prog.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));

    auto incre_info = incre::analysis::buildIncreInfo(prog.get(), env.get());

    // generate single example for each hole
    for (int i = 0; i < 10; ++i) {
        incre_info->example_pool->generateSingleExample();
    }

    // use SuFu to compress solution into scalar values
    auto* solver = new IncreAutoLifterSolver(incre_info, env);
    auto solution = solver->solve();
    std::cout << "zyw: solution.print:" << std::endl;
    solution.print();

    // get the compressed program
    auto result_program = rewriteWithIncreSolution(incre_info->program.get(), solution);
    incre::printProgram(result_program);
    std::cout << std::endl << std::endl;

    // get the type of partial solution
    incre::syntax::Ty solution_ty = incre::syntax::getSolutionType(result_program.get(), ctx);
    std::cout << "zyw: solution_ty = ";
    if (solution_ty) {
        std::cout << solution_ty->toString() << std::endl;
    } else {
        LOG(FATAL) << "solution ty not found!";
    }

    // translate Ty to Type
    PType solution_type = incre::trans::typeFromIncre(solution_ty.get());
    std::cout << "zyw: solution_type = " << solution_type->getName() << std::endl;

    // get object function
    incre::syntax::Term object_func = incre::syntax::getObjFunc(result_program.get(), ctx);
    std::cout << "zyw: object_func = ";
    if (object_func) {
        std::cout << object_func->toString() << std::endl;
    } else {
        LOG(FATAL) << "object function not found!";
    }

    // get DP info
    auto dp_info = incre::analysis::buildIncreInfo(result_program.get(), env.get());

    // generate single example for DP synthesis
    for (int i = 0; i < 10; ++i) {
        dp_info->example_pool->generateDpSingleExample();
    }
    std::cout << std::endl << std::endl;

    // print dp example
    std::cout << "zyw: print DP example" << std::endl;
    std::cout << "dp_example_pool.size() = " << dp_info->example_pool->dp_example_pool.size() << std::endl;
    for (auto& example: dp_info->example_pool->dp_example_pool) {
        std::cout << example->toString() << std::endl;
    }

    
    Data inp = dp_info->example_pool->dp_example_pool[0]->inp;
    Data oup = dp_info->example_pool->dp_example_pool[0]->oup;
    incre::syntax::Term input = std::make_shared<incre::syntax::TmValue>(inp);
    incre::syntax::Term output = std::make_shared<incre::syntax::TmValue>(oup);
    Print(object_func->toString());
    Print(input->toString());
    Print(output->toString());

}