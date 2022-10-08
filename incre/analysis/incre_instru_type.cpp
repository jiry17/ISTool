//
// Created by pro on 2022/9/21.
//

#include "istool/incre/analysis/incre_instru_type.h"
#include "glog/logging.h"

using namespace incre;

VLabeledCompress::VLabeledCompress(const Data &_v, int _id):
    VCompress(_v), id(_id) {
}

TyLabeledCompress::TyLabeledCompress(const Ty &_ty, int _id):
    TyCompress(_ty), id(_id) {
}
std::string TyLabeledCompress::toString() const {
    return "compress[" + std::to_string(id) + "] " + content->toString();
}

TmLabeledCreate::TmLabeledCreate(const Term &_content, int _id):
    TmCreate(_content), id(_id) {
}

TmLabeledPass::TmLabeledPass(const std::vector<std::string> &names, const TermList &_defs, const Term &_content,
        int _tau_id, const std::unordered_map<std::string, Data>& _info):
    TmPass(names, _defs, _content), tau_id(_tau_id), subst_info(_info) {
}

void TmLabeledPass::addSubst(const std::string &name, const Data& data) {
    if (subst_info.find(name) != subst_info.end()) {
        LOG(FATAL) << "Multiple substitution happens for var " << name;
    }
    subst_info[name] = data;
}