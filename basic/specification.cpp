//
// Created by pro on 2021/12/5.
//

#include "specification.h"

SynthInfo::SynthInfo(const std::string &_name, const TypeList &_inp, const PType &_oup, Grammar *_g):
    name(_name), inp_type_list(_inp), oup_type(_oup), grammar(_g) {
}
SynthInfo::~SynthInfo() {
    delete grammar;
}

Specification::Specification(const std::vector<PSynthInfo> &_info_list, TypeSystem *_type_system, Verifier *_v):
    info_list(_info_list), type_system(_type_system), v(_v) {
}

Specification::~Specification() {
    delete type_system; delete v;
}