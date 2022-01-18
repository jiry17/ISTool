//
// Created by pro on 2022/1/18.
//

#include "istool/ext/composed_semantics/composed_semantics.h"

Data ComposedSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    auto pre_inp = info->param_value; info->param_value = inp_list;
    auto res = body->run(info);
    info->param_value = pre_inp;
    return res;
}
ComposedSemantics::ComposedSemantics(const PProgram &_body, int _param_num, const std::string &name):
    FullExecutedSemantics(name.empty() ? _body->toString() : name), body(_body), param_num(_param_num) {
}