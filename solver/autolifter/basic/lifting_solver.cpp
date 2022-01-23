//
// Created by pro on 2022/1/22.
//

#include "istool/solver/autolifter/basic/lifting_solver.h"

LiftingResInfo::LiftingResInfo(const PType &_F, const PProgram &_c, const PProgram &_m): F(_F), c(_c), m(_m) {
}
LiftingRes::LiftingRes(const PProgram &_p, const PProgram &_h, const PProgram &_f, const std::vector<LiftingResInfo> &_info_list):
    p(_p), h(_h), f(_f), info_list(_info_list){
}
SingleLiftingRes::SingleLiftingRes(const PProgram &_p, const PProgram &_h, const PProgram &_f, const LiftingResInfo &_info):
    p(_p), h(_h), f(_f), info(_info) {
}

std::string LiftingRes::toString() const {
    auto res = "f: " + f->toString() + "; c: ";
    for (int i = 0; i < info_list.size(); ++i) {
        if (i) res += ", "; res += info_list[i].c->toString();
    }
    return res;
}
std::string SingleLiftingRes::toString() const {
    return "f: " + f->toString() + "; c: " + info.c->toString();
}

LiftingSolver::LiftingSolver(LiftingTask *_task): task(_task) {}
SFSolver::SFSolver(PartialLiftingTask *_task): task(_task) {}

