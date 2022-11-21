//
// Created by pro on 2022/9/26.
//

#include "istool/incre/autolifter/incre_plp.h"

using namespace incre::autolifter;

PLPTask::PLPTask(FExampleSpace *_example_space, const std::vector<Grammar *> &_f_grammar_list, Grammar *_const_grammar,
        Program *_target, const std::vector<int>& _path, int target_id):
    example_space(_example_space), f_grammar_list(_f_grammar_list), const_grammar(_const_grammar), target(_target), path(_path) {
    target_pos = -1;
    for (auto& [id, _]: example_space->compress_infos) if (id == target_id) target_pos = id;
}

// TODO: add cache
Data PLPTask::runOup(int example_id) {
    return example_space->runOup(example_id, target, path);
}
DataList PLPTask::runInp(int example_id, int id, Program *program) {
    if (id == -1) return {example_space->runConst(example_id, program)};
    return example_space->runAux(example_id, id, program);
}
IOExample PLPTask::getIO(int example_id, const std::vector<std::pair<int, PProgram> > &aux_list) {
    DataList inp;
    for (auto& [aux_id, aux_program]: aux_list) {
        for (auto& d: runInp(example_id, aux_id, aux_program.get())) {
            inp.push_back(d);
        }
    }
    /*if (target_pos != -1) {
        for (auto& d: runInp(example_id, target_pos, target)) {
            inp.push_back(d);
        }
    }*/
    return {inp, runOup(example_id)};
}

int PLPTask::acquireExample(int target_num, int timeout) {
    auto* guard = new TimeGuard(timeout);
    auto res = example_space->acquireExample(target_num, guard);
    delete guard;
    return res;
}