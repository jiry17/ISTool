//
// Created by pro on 2022/9/18.
//

#include "istool/basic/config.h"
#include "istool/basic/grammar.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/language/incre_program.h"
#include "istool/incre/language/incre_dp.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/grammar/incre_grammar_semantics.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/trans/incre_trans.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = ::config::KIncreParserPath + "/benchmarks/01knapsack.f";
    }
    IncreProgram prog = io::parseFromF(path);
    // printProgram(prog);

    auto env = std::make_shared<Env>();
    std::cout << "zyw: env1 = " << std::endl;
    env->printSemanticsPool();

    incre::config::applyConfig(prog.get(), env.get());
    std::cout << "zyw: env2 = " << std::endl;
    env->printSemanticsPool();

    auto ctx = buildContext(prog.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
    
    std::cout << "zyw: ctx->ctx.printTypes" << std::endl;
    ctx->ctx.printTypes();
    std::cout << "zyw: ctx->ctx.printDatas" << std::endl;
    ctx->ctx.printDatas();

    auto incre_info = incre::analysis::buildIncreInfo(prog.get(), env.get());

    std::cout << "zyw: command bind" << std::endl;
    for (auto& command: incre_info->program->commands) {
        auto* cb = dynamic_cast<CommandBindTerm*>(command.get());
        if (cb) {
            std::cout << command->name << ": " << cb->term->toString() << ", " << incre::syntax::termType2String(cb->term->getType()) << std::endl;
        }
    }
    
    std::cout << "zyw: command def" << std::endl;
    for (auto& command: incre_info->program->commands) {
        auto* cb = dynamic_cast<CommandDef*>(command.get());
        if (cb) {
            std::cout << command->name << ": param num = " << cb->param << std::endl;
            for (auto& info : cb->cons_list) {
                std::cout << info.first << ", " << info.second->toString() << std::endl;
            }
        }
    }

    std::cout << "zyw: command declare" << std::endl;
    for (auto& command: incre_info->program->commands) {
        auto* cb = dynamic_cast<CommandDeclare*>(command.get());
        if (cb) {
            std::cout << command->name << ": " << cb->type->toString() << std::endl;
        }
    }

    std::cout << std::endl << "Rewrite infos" << std::endl;
    for (auto& rewrite_info: incre_info->rewrite_info_list) {
        std::cout << rewrite_info.index << " ";
        std::cout << rewrite_info.command_id << " ";
        std::cout << "{"; bool is_start = true;
        for (auto& var_info: rewrite_info.inp_types) {
            if (!is_start) std::cout << ", ";
            std::cout << var_info.first + " @ " + var_info.second->toString();
            is_start = false;
        }
        std::cout << "} -> " << rewrite_info.oup_type->toString() << std::endl;
    }
    std::cout << std::endl;

    for (int i = 0; i < 10; ++i) {
        std::pair<syntax::Term, DataList> tmp = incre_info->example_pool->generateStart();
        std::cout << "Start # " << i << ": term = " << tmp.first->toString() << ", global_inp = ";
        bool is_start = true;
        for (auto& i : tmp.second) {
            if (is_start) is_start = false;
            else std::cout << ", ";
            std::cout << i.toString();
        }
        std::cout << std::endl;
    }
    for (int i = 0; i < 10; ++i) {
        incre_info->example_pool->generateSingleExample();
    }
    for (int index = 0; index < incre_info->rewrite_info_list.size(); ++index) {
        std::cout << "examples collected for #" << index << std::endl;
        for (int i = 0; i < 5 && i < incre_info->example_pool->example_pool[index].size(); ++i) {
            auto& example = incre_info->example_pool->example_pool[index][i];
            std::cout << "  " << example->toString() << std::endl;
        }
    }

    std::cout << "zyw: incre_info->example_pool->cared_vars: " << std::endl;
    incre_info->example_pool->printCaredVars();
    std::cout << "zyw: incre_info->example_pool->start_list: " << std::endl;
    incre_info->example_pool->printStartList();
    std::cout << "zyw: incre_info->example_pool->existing_example_set: " << std::endl;
    incre_info->example_pool->printExistingExampleSet();
    std::cout << "zyw: incre_info->example_pool->global_name_list: " << std::endl;
    incre_info->example_pool->printGlobalNameList();
    std::cout << "zyw: incre_info->example_pool->global_type_list: " << std::endl;
    incre_info->example_pool->printGlobalTypeList();
    std::cout << std::endl;

    // print component_pool
    std::cout << "zyw: incre_info->component_pool: " << std::endl;
    incre_info->component_pool.print();


    // use SuFu to compress solution into scalar values
    auto* solver = new IncreAutoLifterSolver(incre_info, env);
    auto solution = solver->solve();
    std::cout << "zyw: solution.print:" << std::endl;
    solution.print();

    // get the compressed program
    auto result_program = rewriteWithIncreSolution(incre_info->program.get(), solution);
    incre::printProgram(result_program);


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

    // get DP grammar
    std::cout << "zyw: DP - buildDpGrammar" << std::endl;
    TypeList dp_inp_types = std::vector<PType>{solution_type};
    PType dp_oup_type = theory::clia::getTInt();
    Grammar* dp_grammar = incre_info->component_pool.buildDpGrammar(dp_inp_types, dp_oup_type);
    ::grammar::deleteDuplicateRule(dp_grammar);
    dp_grammar->print();
    std::cout << "zyw: DP - buildDpGrammar end" << std::endl << std::endl;

    std::cout << "zyw: DP - generateHeightLimitedGrammar" << std::endl;
    Grammar* dp_grammar_limit = ::grammar::generateHeightLimitedGrammar(dp_grammar, 1);
    dp_grammar_limit->print();
    std::cout << "zyw: DP - generateHeightLimitedGrammar end" << std::endl << std::endl;

    std::cout << "zyw: DP - generateHeightLimitedProgram" << std::endl;
    std::vector<PProgram> dp_program_result = ::grammar::generateHeightLimitedProgram(dp_grammar, 1);
    for (auto& program: dp_program_result) {
        std::cout << program->toString() << std::endl;
    }
    std::cout << "zyw: DP - generateHeightLimitedProgram end" << std::endl << std::endl;

    std::cout << "zyw: DP - postProcessDpProgramList" << std::endl;
    ::grammar::postProcessDpProgramList(dp_program_result);
    for (auto& program: dp_program_result) {
        std::cout << program->toString() << std::endl;
    }
    std::cout << "zyw: DP - postProcessDpProgramList end" << std::endl << std::endl;

    // get BOOL grammar
    std::cout << "zyw: BOOL - buildBoolGrammar" << std::endl;
    TypeList bool_inp_types = std::vector<PType>{theory::clia::getTInt(), theory::clia::getTInt()};
    PType bool_oup_type = type::getTBool();
    Grammar* bool_grammar = incre_info->component_pool.buildBoolGrammar(bool_inp_types, bool_oup_type);
    bool_grammar->print();
    std::cout << "zyw: BOOL - buildBoolGrammar end" << std::endl << std::endl;
    
    std::cout << "zyw: BOOL - generateHeightLimitedGrammar" << std::endl;
    Grammar* bool_grammar_limit = ::grammar::generateHeightLimitedGrammar(bool_grammar, 2);
    bool_grammar_limit->print();
    std::cout << "zyw: BOOL - generateHeightLimitedGrammar end" << std::endl << std::endl;

    std::cout << "zyw: BOOL - generateHeightLimitedProgram" << std::endl;
    std::vector<PProgram> bool_program_result = ::grammar::generateHeightLimitedProgram(bool_grammar, 2);
    for (auto& program: bool_program_result) {
        std::cout << program->toString() << std::endl;
    }
    std::cout << "zyw: BOOL - generateHeightLimitedProgram end" << std::endl << std::endl;

    std::cout << "zyw: BOOL - postProcessBoolProgramList" << std::endl;
    ::grammar::postProcessBoolProgramList(bool_program_result);
    for (auto& program: bool_program_result) {
        std::cout << program->toString() << std::endl;
    }
    std::cout << "zyw: BOOL - postProcessBoolProgramList end" << std::endl << std::endl;

    // merge DP and BOOL
    std::cout << "zyw: MERGE" << std::endl;
    ProgramList merge_program_list = ::grammar::mergeProgramList(dp_program_result, bool_program_result);
    for (auto& program: merge_program_list) {
        std::cout << program->toString() << std::endl;
    }
}