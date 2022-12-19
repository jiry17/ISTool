//
// Created by pro on 2022/12/7.
//

#include "istool/incre/autolabel/incre_autolabel.h"

using namespace incre;
using namespace incre::autolabel;

namespace {
#define ConstructTypeHead(name) Ty _constructType(Ty ## name* type, const Ty& _type, LabelContext* ctx)
#define ConstructTypeCase(name) {res =  _constructType(dynamic_cast<Ty ## name*>(type.get()), type, ctx); break;}

    ConstructTypeHead(Tuple) {
        TyList fields;
        for (auto& field: type->fields) fields.push_back(constructType(field, ctx));
        return std::make_shared<TyTuple>(fields);
    }
    ConstructTypeHead(Compress) {
        return constructType(type->content, ctx);
    }
    ConstructTypeHead(Arrow) {
        auto source = constructType(type->source, ctx);
        auto target = constructType(type->target, ctx);
        return std::make_shared<TyArrow>(source, target);
    }
    ConstructTypeHead(Inductive) {
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (auto& [cname, cty]: type->constructors) {
            cons_list.emplace_back(cname, constructType(cty, ctx));
        }
        return std::make_shared<TyInductive>(type->name, cons_list);
    }
}

Ty autolabel::constructType(const Ty &type, LabelContext *ctx) {
    Ty res;
    switch (type->getType()) {
        case TyType::INT:
        case TyType::BOOL:
        case TyType::UNIT:
            return type;
        case TyType::VAR: {
            res = type; break;
        }
        case TyType::TUPLE: ConstructTypeCase(Tuple)
        case TyType::COMPRESS: ConstructTypeCase(Compress)
        case TyType::IND: ConstructTypeCase(Inductive)
        case TyType::ARROW: ConstructTypeCase(Arrow)
    }
    auto it = ctx->type_compress_map.find(type.get());
    if (it != ctx->type_compress_map.end() && ctx->getLabel(it->second)) {
        return std::make_shared<TyCompress>(res);
    }
    return res;
}