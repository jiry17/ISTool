//
// Created by pro on 2021/12/18.
//

#include "istool/ext/z3/z3_extension.h"
#include "glog/logging.h"

void Z3Extension::registerZ3Type(Z3Type *util) {
    util_list.push_front(util);
}

void Z3Extension::registerZ3Semantics(const std::string &name, Z3Semantics *semantics) {
    semantics_pool[name] = semantics;
}

Z3Type * Z3Extension::getZ3Type(Type *type) const {
    for (auto* util: util_list) {
        if (util->matchType(type)) return util;
    }
    LOG(FATAL) << "Z3 ext: unsupported type " << type->getName();
}

z3::expr Z3Extension::buildConst(const Data &data) {
    auto* util = getZ3Type(data.getType());
    return util->buildConst(data, ctx);
}

z3::expr Z3Extension::buildVar(Type *type, const std::string &name) {
    auto* util = getZ3Type(type);
    return util->buildVar(type, name, ctx);
}

Z3Semantics * Z3Extension::getZ3Semantics(Semantics *semantics) const {
    std::string name = semantics->getName();
    if (semantics_pool.find(name) == semantics_pool.end()) {
        LOG(INFO) << "Z3 ext: unsupported semantics " << name;
    }
    return semantics_pool.find(name)->second;
}

z3::expr Z3Extension::encodeZ3ExprForSemantics(Semantics *semantics, const z3::expr_vector &inp_list, const z3::expr_vector &param_list) {
    auto* ps = dynamic_cast<ParamSemantics*>(semantics);
    if (ps) {
        return param_list[ps->id];
    }
    auto* cs = dynamic_cast<ConstSemantics*>(semantics);
    if (cs) {
        return buildConst(cs->w);
    }
    auto* zs = getZ3Semantics(semantics);
    return zs->encodeZ3Expr(inp_list);
}

z3::expr Z3Extension::encodeZ3ExprForProgram(Program *program, const z3::expr_vector &param_list) {
    z3::expr_vector inp_list(ctx);
    for (const auto& sub: program->sub_list) {
        inp_list.push_back(encodeZ3ExprForProgram(sub.get(), param_list));
    }
    return encodeZ3ExprForSemantics(program->semantics.get(), inp_list, param_list);
}

Data Z3Extension::getValueFromModel(const z3::model &model, const z3::expr &expr, Type *type, bool is_strict) {
    auto* util = getZ3Type(type);
    return util->getValueFromModel(model, expr, type, is_strict);
}

Z3Extension::~Z3Extension() {
    for (auto* util: util_list) delete util;
    for (auto& sem_info: semantics_pool) {
        delete sem_info.second;
    }
}


const std::string KZ3Name = "Z3";

Z3Extension * ext::z3::getZ3Extension(Env *env) {
    auto* res = env->getExtension(KZ3Name);
    if (res) {
        auto* z3_ext = dynamic_cast<Z3Extension*>(res);
        if (!z3_ext) {
            LOG(FATAL) << "Unmatched Extension " << KZ3Name;
        }
        return z3_ext;
    }
    auto* z3_ext = new Z3Extension();
    env->registerExtension(KZ3Name, z3_ext);
    return z3_ext;
}
