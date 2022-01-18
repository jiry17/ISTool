//
// Created by pro on 2022/1/15.
//

#include "istool/ext/deepcoder/anonymous_function.h"
#include "glog/logging.h"

AnonymousSemantics::AnonymousSemantics(const SemanticsFunction &_f, const std::string &_name): f(_f), Semantics(_name) {
}
AnonymousSemantics::AnonymousSemantics(const FullSemanticsFunction &_f, const std::string &_name): Semantics(_name) {
    f = [_f](const ProgramList& sub_list, ExecuteInfo* info) -> Data {
        DataList res;
        for (const auto& sub: sub_list) res.push_back(sub->run(info));
        return _f(std::move(res), info);
    };
}
Data AnonymousSemantics::run(const std::vector<std::shared_ptr<Program> > &sub_list, ExecuteInfo *info) {
    return f(sub_list, info);
}

Data ext::ho::buildAnonymousData(const SemanticsFunction &f, const std::string &name) {
    auto as = std::make_shared<AnonymousSemantics>(f, name);
    return Data(std::make_shared<SemanticsValue>(as));
}
Data ext::ho::buildAnonymousData(const FullSemanticsFunction &f, const std::string &name) {
    auto as = std::make_shared<AnonymousSemantics>(f, name);
    return Data(std::make_shared<SemanticsValue>(as));
}
PSemantics ext::ho::getSemantics(const Data &data) {
    auto* sv = dynamic_cast<SemanticsValue*>(data.get());
    if (!sv) {
        LOG(FATAL) << "Expect SemanticsValue but get " << data.toString();
    }
    return sv->sem;
}