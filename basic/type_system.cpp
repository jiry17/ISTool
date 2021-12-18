//
// Created by pro on 2021/12/4.
//

#include "type_system.h"

PType DummyTypeSystem::getType(const PProgram &program) const {
    return std::make_shared<TBot>();
}
PType DummyTypeSystem::typeCheck(const PProgram &program) const {
    return std::make_shared<TBot>();
}

BasicTypeSystem::BasicTypeSystem(const VTAssigner &_assigner): value_assigner(_assigner) {
}

PType BasicTypeSystem::getType(const PProgram &program) const {
    auto* ps = dynamic_cast<ParamSemantics*>(program->semantics.get());
    if (ps) {
        auto* vt = dynamic_cast<TVar*>(ps->oup_type.get());
        if (vt) return nullptr;
        return ps->oup_type;
    }
    auto* cs = dynamic_cast<ConstSemantics*>(program->semantics.get());
    if (cs) {
        return value_assigner(cs->w);
    }
    auto* ts = dynamic_cast<TypedSemantics*>(program->semantics.get());
    if (!ts) return nullptr;
    return ts->oup_type;
}

PType BasicTypeSystem::typeCheck(const PProgram &program) const {
    auto oup_type = getType(program);
    auto* ts = dynamic_cast<TypedSemantics*>(program->semantics.get());
    TypeList sub_types;
    for (const auto& sub: program->sub_list) {
        auto res = getType(sub);
        if (!res) return nullptr;
        sub_types.push_back(res);
    }
    if (sub_types.size() != ts->inp_type_list.size()) return nullptr;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (!type::equal(sub_types[i], ts->inp_type_list[i])) {
            return nullptr;
        }
    }
    return oup_type;
}