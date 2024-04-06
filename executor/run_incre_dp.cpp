//
// Created by pro on 2022/9/18.
//

#include "istool/basic/data.h"
#include "istool/basic/config.h"
#include "istool/basic/grammar.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/language/incre_dp.h"
#include "istool/incre/grammar/incre_grammar_semantics.h"
#include "istool/incre/io/incre_from_json.h"
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

    // get the SuFU-result program
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

    // config for building grammar
    int dp_height_limit = 2;
    int bool_height_limit = 1;
    int dp_bool_height_limit = 3;
    int single_example_num = 10;
    ProgramList program_list;

    // get DP_BOOL grammar
    std::cout << "zyw: DP_BOOL - buildDpBoolGrammar" << std::endl;
    TypeList dp_bool_inp_types = std::vector<PType>{solution_type, solution_type};
    PType dp_bool_oup_type = type::getTBool();
    Grammar* dp_bool_grammar = incre_info->component_pool.buildDpBoolGrammar(dp_bool_inp_types, dp_bool_oup_type);
    ::grammar::deleteDuplicateRule(dp_bool_grammar);
    dp_bool_grammar->print();
    std::cout << "zyw: DP_BOOL - buildDpBoolGrammar end" << std::endl << std::endl;

    std::cout << "zyw: DP_BOOL - generateHeightLimitedProgram" << std::endl;
    std::vector<PProgram> dp_bool_program_result = ::grammar::generateHeightLimitedProgram(dp_bool_grammar, dp_bool_height_limit);
    ::grammar::postProcessDpBoolProgramList(dp_bool_program_result);
    std::cout << "program.size = " << dp_bool_program_result.size() << std::endl;
    for (auto& program: dp_bool_program_result) {
        std::cout << program->toString() << std::endl;
    }
    program_list = dp_bool_program_result;
    std::cout << "zyw: DP_BOOL - generateHeightLimitedProgram end" << std::endl << std::endl;

    // get object function
    incre::syntax::Term object_func = incre::syntax::getObjFunc(result_program.get(), ctx);
    if (object_func) {
        std::cout << "zyw: object_func = " << object_func->toString() << std::endl;
    } else {
        LOG(FATAL) << "object function not found!";
    }
    std::cout << std::endl << std::endl;

    // get DP info
    auto dp_info = incre::analysis::buildIncreInfo(result_program.get(), env.get());

    // generate single example for DP synthesis
    for (int i = 0; i < single_example_num; ++i) {
        dp_info->example_pool->generateDpSingleExample();
    }
    std::cout << std::endl << std::endl;

    // print dp example
    std::cout << "zyw: print DP example" << std::endl;
    std::cout << "dp_example_pool.size() = " << dp_info->example_pool->dp_example_pool.size() << std::endl;
    for (auto& example: dp_info->example_pool->dp_example_pool) {
        std::cout << example->toString() << std::endl;
    }
    std::cout << std::endl << std::endl;

    // single example -> DP solution data
    std::cout << "zyw: print DP solution set" << std::endl;
    incre::example::DpSolutionSet dp_sol_set;
    for (auto& example: dp_info->example_pool->dp_example_pool) {
        dp_sol_set.add(example);
    }
    Print(dp_sol_set.sol_list.size());
    for (auto& sol: dp_sol_set.sol_list) {
        std::cout << sol->partial_solution.toString() << ", child size = " << sol->children.size() << std::endl;
        for (auto& child: sol->children) {
            std::cout << "  " << child->toString() << std::endl;
        }
    }
    std::cout << std::endl << std::endl;

    // apply object function on partial solution
    std::cout << "zyw: apply object function on partial solution" << std::endl;
    incre::semantics::DefaultEvaluator default_eval = incre::semantics::DefaultEvaluator();
    auto new_ctx = buildContext(result_program.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
    std::vector<int> apply_obj_func_result;
    for (auto& sol: dp_sol_set.sol_list) {
        Data par_sol = sol->partial_solution;
        Data new_term_result = incre::syntax::applyObjFunc(object_func, par_sol, default_eval, new_ctx);
        apply_obj_func_result.push_back(::data::getIntFromData(new_term_result));
        int int_value_result = ::data::getIntFromData(new_term_result);
        Print(par_sol.toString());
        Print(new_term_result.toString());
        Print(int_value_result);
        std::cout << std::endl;
    }
    std::cout << std::endl << std::endl;

    // calculate sol relation using R
    FunctionContext func_ctx;
    int sol_len = dp_sol_set.sol_list.size();
    int program_len = program_list.size();
    // whether is the program consistent with the thinning theorem
    std::vector<bool> program_flag = std::vector<bool>(program_len, true);
    for (int k = 0; k < program_len; ++k) {
        PProgram r_program = program_list[k];
        Print(r_program->toString());
        for (int i = 0; i < sol_len; ++i) {
            if (!program_flag[k]) break;
            for (int j = 0; j < sol_len; ++j) {
                if (!program_flag[k]) break;
                
                Data sol_1 = dp_sol_set.sol_list[i]->partial_solution;
                Data sol_2 = dp_sol_set.sol_list[j]->partial_solution;
                int obj_result_1 = apply_obj_func_result[i];
                int obj_result_2 = apply_obj_func_result[j];
                Data r_result = incre::syntax::calRelation(r_program, sol_1, sol_2, func_ctx);
                bool bool_r_result = ::data::getBoolFromData(r_result);

                // apply thinning theorem-1
                if (obj_result_1 > obj_result_2) {
                    // if obj_1 >= obj_2, then must have (sol_1 R sol_2) = false
                    if (bool_r_result) {
                        // not consistent with the tinning theorem - Optimality
                        program_flag[k] = false;
                        Print(sol_1.toString());
                        Print(sol_2.toString());
                        Print(obj_result_1);
                        Print(obj_result_2);
                        Print(bool_r_result);
                        std::cout << std::endl;
                    }
                }
                
                // apply thinning theorem-2
                if (bool_r_result) {
                    bool child_result = true;
                    for (auto& child_1: dp_sol_set.sol_list[i]->children) {
                        bool has_y = false;
                        for (auto& child_2: dp_sol_set.sol_list[j]->children) {
                            Data tmp_result = incre::syntax::calRelation(r_program, child_1->partial_solution, child_2->partial_solution, func_ctx);
                            if (::data::getBoolFromData(tmp_result)) {
                                has_y = true;
                                break;
                            }
                        }
                        if (!has_y) {
                            child_result = false;
                            program_flag[k] = false;
                            Print(sol_1.toString());
                            Print(sol_2.toString());
                            Print(child_1->partial_solution.toString());
                            std::cout << std::endl;
                            break;
                        }
                    }
                }
            }
        }
    }

    int result_num = 0;
    for (int k = 0; k < program_len; ++k) {
        if (program_flag[k]) result_num++;
    }
    Print(result_num);

    for (int k = 0; k < program_len; ++k) {
        if (program_flag[k]) {
            Print(program_list[k]->toString());
        }
    }
    
    std::cout << std::endl << std::endl;

    // result for 01knapsack
    // &&(<=(Param1.0,Param0.0),<=(Param0.1,Param1.1))
    // &&(<(Param1.0,Param0.0),<(Param0.1,Param1.1))

    // print time record
    global::recorder.printAll();
}