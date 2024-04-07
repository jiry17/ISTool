//
// Created by zyw on 2024/3/29.
//

#include "istool/basic/data.h"
#include "istool/basic/config.h"
#include "istool/basic/grammar.h"
#include "istool/basic/time_guard.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/language/incre_dp.h"
#include "istool/incre/grammar/incre_grammar_semantics.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/trans/incre_trans.h"
#include <iostream>
#include <tuple>
#include "glog/logging.h"

using namespace incre;
#define Print(x) std::cout << #x " = " << x << std::endl;
#define Print_n(n) for (int i = 0; i < n; ++i) std::cout << std::endl;

int main(int argv, char** argc) {
    std::string path, target;

    global::recorder.start("all");
    global::recorder.start("1-SuFu");
    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = ::config::KIncreParserPath + "/benchmark/01knapsack.f";
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

    // config for building grammar
    int sample_size = 7;
    int dp_bool_height_limit = 3;
    int single_example_num = 8;
    ProgramList program_list;

    // get the SuFU-result program
    auto result_program = rewriteWithIncreSolution(incre_info->program.get(), solution);
    incre::printProgram(result_program);
    std::shared_ptr<Value> size_value = result_program->config_map[IncreConfig::SAMPLE_SIZE].value;
    std::shared_ptr<IntValue> int_size_value = std::static_pointer_cast<IntValue>(size_value);
    if (int_size_value) {
        Print(int_size_value->w);
        int_size_value->w = sample_size;
        Print(int_size_value->w);
    } else {
        LOG(FATAL) << "config_map[sample_size] is not IntValue";
    }
    Print_n(2);
    global::recorder.end("1-SuFu");

    global::recorder.start("2-get type of partial solution");
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
    global::recorder.end("2-get type of partial solution");

    // get DP_BOOL grammar
    global::recorder.start("3-get DP_BOOL grammar");
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
    Print(program_list.size());
    global::recorder.end("3-get DP_BOOL grammar");

    // get object function
    global::recorder.start("4-get object function");
    incre::syntax::Term object_func = incre::syntax::getObjFunc(result_program.get(), ctx);
    if (object_func) {
        std::cout << "zyw: object_func = " << object_func->toString() << std::endl;
    } else {
        LOG(FATAL) << "object function not found!";
    }
    Print_n(2);
    global::recorder.end("4-get object function");

    // get DP info
    auto dp_incre_info = incre::analysis::buildIncreInfo(result_program.get(), env.get());
    // number of programs
    int program_len = program_list.size();
    // number of remaining programs
    int program_remain_num = program_list.size();
    // whether is the program consistent with the thinning theorem
    std::vector<bool> program_flag = std::vector<bool>(program_len, true);
    // number of result = true applying on dp_deduplicate_sol_set
    std::vector<int> program_true_num = std::vector<int>(program_len, 0);
    // number of result = true applying on dp_same_time_solution_list
    std::vector<int> program_same_time_true_num = std::vector<int>(program_len, 0);
    // number of programs deleted by each thinning theorem
    int delete_num_all_1 = 0, delete_num_all_2 = 0;

    // dp solutions for each cycle
    incre::example::DpSolutionList dp_example_pool;
    // dp solution pairs for each cycle, used in thinning theorem-2
    std::vector<std::pair<incre::example::DpSolution, incre::example::DpSolution>> dp_same_time_solution;
    // all dp solution pairs, used in calculate r_true_num
    std::vector<std::vector<std::pair<incre::example::DpSolution, incre::example::DpSolution>>> dp_same_time_solution_list;
    // deduplicated solutions in all cycles, used in thinning theorem-1
    std::unordered_set<std::string> existing_dp_sol;
    std::vector<incre::example::DpSolution> dp_deduplicate_sol_set;
    int dp_deduplicate_sol_len_pre = 0;
    int dp_deduplicate_sol_len_now = 0;
    // result of applying object function on dp_deduplicate_sol_set
    std::vector<int> object_result_list; 
    
    // used in apply object function
    FunctionContext func_ctx;
    incre::semantics::DefaultEvaluator default_eval = incre::semantics::DefaultEvaluator();
    auto new_ctx = buildContext(result_program.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
    
    global::recorder.start("5-filter R");
    for (int i = 0; i < single_example_num; ++i) {
        std::cout << "zyw: start of cycle " << i << std::endl;
        TimeRecorder partial_recorder;
        partial_recorder.start("cycle time");

        int delete_num_1 = 0, delete_num_2 = 0;
        
        global::recorder.start("5.1-generate single example");
        partial_recorder.start("5.1-generate single example");
        // clear example_pool
        dp_incre_info->example_pool->clear();
        // generate single example for DP synthesis
        dp_incre_info->example_pool->generateDpSingleExample();
        partial_recorder.end("5.1-generate single example");
        global::recorder.end("5.1-generate single example");

        global::recorder.start("5.2-get partial solutions");
        partial_recorder.start("5.2-get partial solutions");
        // get partial solutions
        dp_example_pool = dp_incre_info->example_pool->dp_example_pool;
        dp_same_time_solution = dp_incre_info->example_pool->dp_same_time_solution;
        dp_same_time_solution_list.push_back(dp_same_time_solution);
        
        // get all solutions -> dp_deduplicate_sol_set, object_result_list
        for (auto& sol: dp_example_pool) {
            std::string sol_str = sol->toString();
            if (existing_dp_sol.find(sol_str) == existing_dp_sol.end()) {
                existing_dp_sol.insert(sol_str);
                dp_deduplicate_sol_set.push_back(sol);
                Data tmp = incre::syntax::applyObjFunc(object_func, sol->partial_solution, default_eval, new_ctx);
                int obj_result = ::data::getIntFromData(tmp);
                object_result_list.push_back(obj_result);
            }
        }
        if (dp_deduplicate_sol_set.size() != object_result_list.size()) {
            LOG(FATAL) << "dp_deduplicate_sol_set.size() != object_result_list.size()";
        }
        dp_deduplicate_sol_len_now = dp_deduplicate_sol_set.size();

        // print deduplicated dp solution set
        Print(dp_deduplicate_sol_set.size());
        for (auto& sol: dp_deduplicate_sol_set) {
            std::cout << sol->partial_solution.toString() << std::endl;
        }

        // print all solutions and their children
        Print(dp_example_pool.size());
        for (auto& sol: dp_example_pool) {
            std::cout << sol->partial_solution.toString() << ", child size = " << sol->children.size() << std::endl;
            for (auto& child: sol->children) {
                std::cout << "  " << child->toString() << std::endl;
            }
        }
        Print_n(2);
        partial_recorder.end("5.2-get partial solutions");
        global::recorder.end("5.2-get partial solutions");

        // calculate sol relation using R
        for (int k = 0; k < program_len; ++k) {
            if (!program_flag[k]) continue;
            PProgram r_program = program_list[k];
            // delta num of program_true_num for program_list[k]
            int program_true_num_delta = 0;
            // delta num of program_same_time_true_num for program_list[k]
            int program_same_time_true_num_delta = 0;

            global::recorder.start("5.3-thinning theorem-1");
            partial_recorder.start("5.3-thinning theorem-1");
            // apply thinning theorem-1
            for (int i = 0; i < dp_deduplicate_sol_len_now; ++i) {
                if (!program_flag[k]) break;
                for (int j = dp_deduplicate_sol_len_pre; j < dp_deduplicate_sol_len_now; ++j) {
                    if (!program_flag[k]) break;

                    incre::example::DpSolution sol_1 = dp_deduplicate_sol_set[i];
                    incre::example::DpSolution sol_2 = dp_deduplicate_sol_set[j];
                    int obj_result_1 = object_result_list[i];
                    int obj_result_2 = object_result_list[j];
                    // if obj_1 >= obj_2, then must have (sol_1 R sol_2) = false
                    if (!(obj_result_1 >= obj_result_2)) {
                        global::recorder.start("5.3.1-calculate program_true_num for none use");
                    }
                    Data r_result = incre::syntax::calRelation(r_program, sol_1->partial_solution, sol_2->partial_solution, func_ctx);
                    bool bool_r_result = ::data::getBoolFromData(r_result);
                    if (bool_r_result) {
                        program_true_num_delta++;
                    }
                    if (!(obj_result_1 >= obj_result_2)) {
                        global::recorder.end("5.3.1-calculate program_true_num for none use");
                    }
                    if (obj_result_1 >= obj_result_2) {
                        if (bool_r_result) {
                            // not consistent with the tinning theorem - Optimality
                            program_flag[k] = false;
                            program_remain_num--;
                            delete_num_1 += 1;
                            Print(r_program->toString());
                            Print(sol_1->toString());
                            Print(sol_2->toString());
                            Print(obj_result_1);
                            Print(obj_result_2);
                            Print(bool_r_result);
                            Print_n(1);
                        }
                    }
                }
            }
            for (int i = dp_deduplicate_sol_len_pre; i < dp_deduplicate_sol_len_now; ++i) {
                if (!program_flag[k]) break;
                for (int j = 0; j < dp_deduplicate_sol_len_now; ++j) {
                    if (!program_flag[k]) break;

                    incre::example::DpSolution sol_1 = dp_deduplicate_sol_set[i];
                    incre::example::DpSolution sol_2 = dp_deduplicate_sol_set[j];
                    int obj_result_1 = object_result_list[i];
                    int obj_result_2 = object_result_list[j];
                    // if obj_1 >= obj_2, then must have (sol_1 R sol_2) = false
                    if (!(obj_result_1 >= obj_result_2)) {
                        global::recorder.start("5.3.1-calculate program_true_num for none use");
                    }
                    Data r_result = incre::syntax::calRelation(r_program, sol_1->partial_solution, sol_2->partial_solution, func_ctx);
                    bool bool_r_result = ::data::getBoolFromData(r_result);
                    if (bool_r_result) {
                        program_true_num_delta++;
                    }
                    if (!(obj_result_1 >= obj_result_2)) {
                        global::recorder.end("5.3.1-calculate program_true_num for none use");
                    }
                    if (obj_result_1 >= obj_result_2) {
                        if (bool_r_result) {
                            // not consistent with the tinning theorem - Optimality
                            program_flag[k] = false;
                            program_remain_num--;
                            delete_num_1 += 1;
                            Print(r_program->toString());
                            Print(sol_1->toString());
                            Print(sol_2->toString());
                            Print(obj_result_1);
                            Print(obj_result_2);
                            Print(bool_r_result);
                            Print_n(1);
                        }
                    }
                }
            }
            program_true_num[k] += program_true_num_delta;
            partial_recorder.end("5.3-thinning theorem-1");
            global::recorder.end("5.3-thinning theorem-1");

            global::recorder.start("5.4-thinning theorem-2");
            partial_recorder.start("5.4-thinning theorem-2");
            // apply thinning theorem-2 
            for (auto& sol_pair: dp_same_time_solution) {
                if (!program_flag[k]) break;

                incre::example::DpSolution sol_1 = sol_pair.first;
                incre::example::DpSolution sol_2 = sol_pair.second;
                Data tmp_1 = incre::syntax::applyObjFunc(object_func, sol_1->partial_solution, default_eval, new_ctx);
                Data tmp_2 = incre::syntax::applyObjFunc(object_func, sol_2->partial_solution, default_eval, new_ctx);
                int obj_result_1 = ::data::getIntFromData(tmp_1);
                int obj_result_2 = ::data::getIntFromData(tmp_2);
                Data r_result = incre::syntax::calRelation(r_program, sol_1->partial_solution, sol_2->partial_solution, func_ctx);
                bool bool_r_result = ::data::getBoolFromData(r_result);
                
                // if sol_1 R sol_2 = true
                if (bool_r_result) {
                    program_same_time_true_num_delta++;

                    bool child_result = true;
                    for (auto& child_1: sol_1->children) {
                        bool has_y = false;
                        for (auto& child_2: sol_2->children) {
                            Data tmp_result = incre::syntax::calRelation(r_program, child_1->partial_solution, child_2->partial_solution, func_ctx);
                            if (::data::getBoolFromData(tmp_result)) {
                                has_y = true;
                                break;
                            }
                        }
                        // if doesn't have sol_y', then not consistent with the tinning theorem-2
                        if (!has_y) {
                            child_result = false;
                            program_flag[k] = false;
                            program_remain_num--;
                            delete_num_2 += 1;
                            Print(r_program->toString());
                            Print(sol_1->toString());
                            Print(sol_2->toString());
                            Print(child_1->partial_solution.toString());
                            Print_n(1);
                            break;
                        }
                    }
                }
            }
            program_same_time_true_num[k] += program_same_time_true_num_delta;
            partial_recorder.end("5.4-thinning theorem-2");
            global::recorder.end("5.4-thinning theorem-2");
        }
        // update solution len
        Print(dp_deduplicate_sol_len_pre);
        Print(dp_deduplicate_sol_len_now);
        dp_deduplicate_sol_len_pre = dp_deduplicate_sol_len_now;

        // print dp_same_time_size
        Print(dp_same_time_solution.size());

        // update delete_num
        Print(delete_num_1);
        Print(delete_num_2);
        Print(program_remain_num);
        delete_num_all_1 += delete_num_1;
        delete_num_all_2 += delete_num_2;

        partial_recorder.end("cycle time");
        partial_recorder.printAll();
        Print_n(2);
    }
    global::recorder.end("5-filter R");

    // print all satisfied programs, calculate r_true_num for each program
    global::recorder.start("6-get the max_num program");
    std::cout << "zyw: get the max_num program" << std::endl;
    std::vector<std::tuple<PProgram, int, int>> result_program_list;
    int r_true_num_max = 0;
    PProgram num_max_program;
    int delete_num_true_num = 0;
    int r_true_num_max_2 = 0;
    PProgram num_max_program_2;
    int delete_num_same_time_true_num = 0;
    for (int k = 0; k < program_len; ++k) {
        if (program_flag[k]) {
            PProgram r_program = program_list[k];
            if (!num_max_program) num_max_program = r_program;
            if (!num_max_program_2) num_max_program_2 = r_program;
            int r_true_num = program_true_num[k];
            int r_same_time_true_num = program_same_time_true_num[k];

            if (r_true_num == 0) {
                delete_num_true_num++;
            }
            if (r_same_time_true_num == 0) {
                delete_num_same_time_true_num++;
            }
            if (r_true_num > 0 && r_same_time_true_num > 0) {
                result_program_list.push_back(std::make_tuple(r_program, r_true_num, r_same_time_true_num));
            }

            if (r_true_num > r_true_num_max || r_true_num == r_true_num_max && r_program->toString().length() > num_max_program->toString().length()) {
                num_max_program = r_program;
                r_true_num_max = r_true_num;
            }
            if (r_same_time_true_num > r_true_num_max_2 || r_true_num_max_2 == r_same_time_true_num && r_program->toString().length() > num_max_program_2->toString().length()) {
                num_max_program_2 = r_program;
                r_true_num_max_2 = r_same_time_true_num;
            }
            
            Print(r_program->toString());
            Print(r_true_num);
            Print(r_same_time_true_num);
        }
    }
    Print_n(2);

    // print result_program_list
    std::cout << "zyw: print result_program_list" << std::endl;
    for (auto& [program, a, b]: result_program_list) {
        Print(program->toString());
        Print(a);
        Print(b);
    }
    Print_n(2);
    Print(dp_deduplicate_sol_set.size());
    Print(program_len);
    Print(delete_num_all_1);
    Print(delete_num_all_2);
    Print(delete_num_true_num);
    Print(delete_num_same_time_true_num);
    Print(result_program_list.size());
    if (num_max_program) {
        Print(num_max_program->toString());
        Print(r_true_num_max);
    }
    if (num_max_program_2) {
        Print(num_max_program_2->toString());
        Print(r_true_num_max_2);
    }
    Print_n(2);
    global::recorder.end("6-get the max_num program");

    // result for 01knapsack
    // &&(<=(Param1.0,Param0.0),<=(Param0.1,Param1.1))
    // &&(<(Param1.0,Param0.0),<(Param0.1,Param1.1))

    global::recorder.end("all");
    // print time record
    global::recorder.printAll();
}