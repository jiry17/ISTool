//
// Created by pro on 2022/9/18.
//

#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/autolifter/incre_aux_semantics.h"
#include "istool/incre/language/incre_program.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include <iostream>

using namespace incre;

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = ::config::KIncreParserPath + "/test.f";
    }
    auto prog = io::parseFromF(path);

    auto env = std::make_shared<Env>();
    incre::config::applyConfig(prog.get(), env.get());

    auto ctx = buildContext(prog.get(), BuildGen(incre::semantics::DefaultEvaluator),
                            BuildGen(types::DefaultIncreTypeChecker), BuildGen(syntax::IncreTypeRewriter));
    auto incre_info = incre::analysis::buildIncreInfo(prog.get(), env.get());
    for (auto& command: incre_info->program->commands) {
        auto* cb = dynamic_cast<CommandBind*>(command.get());
        if (cb) {
            std::cout << command->name << ": " << cb->term->toString() << std::endl;
        }
    }

    std::cout << std::endl << "Rewrite infos" << std::endl;
    for (auto& rewrite_info: incre_info->rewrite_info_list) {
        std::cout << "{"; bool is_start = true;
        for (auto& var_info: rewrite_info.inp_types) {
            if (!is_start) std::cout << ",";
            std::cout << var_info.first + "@" + var_info.second->toString();
            is_start = false;
        }
        std::cout << "} -> " << rewrite_info.oup_type->toString() << std::endl;
    }
    std::cout << std::endl;

    for (int i = 0; i < 10; ++i) {
        incre_info->pool->generateSingleExample();
    }
    for (int index = 0; index < incre_info->rewrite_info_list.size(); ++index) {
        std::cout << "examples collected for #" << index << std::endl;
        for (int i = 0; i < 10 && i < incre_info->pool->example_pool[index].size(); ++i) {
            auto& example = incre_info->pool->example_pool[index][i];
            std::cout << "  " << example->toString() << std::endl;
        }
    }

    /*auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());
    auto* info = incre::buildIncreInfo(prog, env.get());
    for (int i = 1; i <= 100; ++i) {
        info->example_pool->generateSingleExample();
    }

    for (auto& align_info: info->align_infos) {
        align_info->print();
        if (align_info->getId() >= info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[align_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }

    auto* solver = new incre::IncreAutoLifterSolver(info, env);
    auto solution = solver->solve();
    solution.print();*/
}