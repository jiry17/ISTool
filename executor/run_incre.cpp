//
// Created by pro on 2022/9/18.
//

#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/language/incre_program.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/grammar/incre_grammar_semantics.h"
#include "istool/incre/io/incre_printer.h"
#include <iostream>
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    // 初始化 Glog
    google::InitGoogleLogging("/home/jiry/zyw/dp/ISTool/build/executor/run_incre");
    // 设置日志文件输出目录
    FLAGS_log_dir = "/home/jiry/zyw/dp/ISTool/log";
    // 设置日志文件名（可选）
    FLAGS_logbufsecs = 0;  // 立即刷新到文件
    
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = ::config::KIncreParserPath + "/benchmarks/mts.f";
    }
    IncreProgram prog = io::parseFromF(path);
    // printProgram(prog);

    auto env = std::make_shared<Env>();
    incre::config::applyConfig(prog.get(), env.get());

    auto ctx = buildContext(prog.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
    std::cout << "zyw: ctx->ctx.printTypes begin" << std::endl;
    ctx->ctx.printTypes();
    std::cout << "zyw: ctx->ctx.printTypes end" << std::endl;
    auto incre_info = incre::analysis::buildIncreInfo(prog.get(), env.get());

    std::cout << "zyw: command bind begin" << std::endl;
    for (auto& command: incre_info->program->commands) {
        auto* cb = dynamic_cast<CommandBindTerm*>(command.get());
        if (cb) {
            std::cout << command->name << ": " << cb->term->toString() << ", " << incre::syntax::termType2String(cb->term->getType()) << std::endl;
        }
    }
    
    std::cout << "zyw: command def begin" << std::endl;
    for (auto& command: incre_info->program->commands) {
        auto* cb = dynamic_cast<CommandDef*>(command.get());
        if (cb) {
            std::cout << command->name << ": " << cb->param << std::endl;
        }
    }

    std::cout << "zyw: command declare begin" << std::endl;
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
        std::cout << "Start # " << i << ": " << incre_info->example_pool->generateStart().first->toString() << std::endl;
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

    std::cout << std::endl;

    // incre_info->component_pool.print();

    auto* solver = new IncreAutoLifterSolver(incre_info, env);
    auto res = solver->solve();
    std::cout << "zyw: res.print:" << std::endl;
    res.print();
    // auto res_prog = rewriteWithIncreSolution(prog.get(), res);
    

    // 关闭 Glog
    google::ShutdownGoogleLogging();
}