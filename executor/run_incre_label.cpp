//
// Created by pro on 2022/9/18.
//


#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/solver/polygen/lia_solver.h"
#include "istool/incre/autolabel/incre_autolabel_constraint_solver.h"
#include "istool/incre/autolifter/incre_nonscalar_autolifter.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/autolifter/incre_nonscalar_oc.h"
#include "istool/incre/grammar/incre_component_collector.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>

using namespace incre;

DEFINE_string(benchmark, "/usr/jiry/ISTool/incre-tests/syc.f", "The absolute path of the benchmark file (.sl)");
DEFINE_string(output, "", "The absolute path of the output file");
DEFINE_bool(autolabel, true, "Whether automatically generate annotations");
DEFINE_bool(scalar, true, "Whether consider only scalar expressions when filling sketch holes");
DEFINE_bool(mark_rewrite, false, "Whetehr to mark the sketch holes.");
DEFINE_string(stage_output_file, "", "Only used in online demo");

int main(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::string path = FLAGS_benchmark, target = FLAGS_output;
    bool is_autolabel = FLAGS_autolabel, is_scalar = FLAGS_scalar, is_mark = FLAGS_mark_rewrite;
    global::KStageInfoPath = FLAGS_stage_output_file;

    TimeGuard* global_guard = new TimeGuard(1e9);

    auto env = std::make_shared<Env>();
    incre::prepareEnv(env.get());
    env->setConst(solver::lia::KIsGurobiName, BuildData(Bool, false));

    auto input_program = incre::parseFromF(path, is_autolabel);
    // init_program = incre::removeGlobal(init_program.get());

    IncreProgram labeled_program;

    if (is_autolabel) {
        global::printStageResult("Start stage 0/2: generating annotations.");
        global::recorder.start("label");
        auto *label_solver = new autolabel::AutoLabelZ3Solver(input_program);
        labeled_program = label_solver->label();
        global::recorder.end("label");
        global::printStageResult("Stage 0/2 Finished");
    } else {
        labeled_program = input_program;
    }
    incre::applyConfig(labeled_program.get(), env.get());

    labeled_program = incre::eliminateNestedAlign(labeled_program.get());
    incre::printProgram(labeled_program);
    env->setConst(incre::grammar::collector::KCollectMethodName, BuildData(Int, incre::grammar::ComponentCollectorType::SOURCE));
    env->setConst(theory::clia::KINFName, BuildData(Int, 50000));

    auto* info = incre::buildIncreInfo(labeled_program, env.get());

    for (auto& align_info: info->align_infos) {
        align_info->print();
        if (align_info->getId() >= info->example_pool->example_pool.size()) continue;
        auto& example_list = info->example_pool->example_pool[align_info->getId()];
        for (int i = 0; i < 10 && i < example_list.size(); ++i) {
            std::cout << "  " << example_list[i]->toString() << std::endl;
        }
    }

    IncreSolution solution;

    if (is_scalar) {
        auto* solver = new incre::IncreAutoLifterSolver(info, env);
        solution = solver->solve();
    } else {
        auto* runner = new autolifter::OCRunner(info, env.get(), 2, 1);
        autolifter::NonScalarAlignSolver* aux_solver = new autolifter::OCNonScalarAlignSolver(info, runner);
        auto *ns_solver = new autolifter::IncreNonScalarSolver(info, env, aux_solver);

        solution = ns_solver->solve();
    }
    solution.print();
    // LOG(INFO) << "After execute time " << global::recorder.query("execute");

    auto full_res = incre::rewriteWithIncreSolution(info->program.get(), solution, env.get(), is_mark);
    full_res = incre::eliminateUnusedLet(full_res.get());

    if (!target.empty()) incre::printProgram(full_res, target, is_mark);
    incre::printProgram(full_res, {}, is_mark);

    global::recorder.printAll();
    std::cout << global_guard->getPeriod() << std::endl;
    std::cout << "Success" << std::endl;
}