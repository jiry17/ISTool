//
// Created by pro on 2023/1/22.
//

#include "istool/incre/language/incre.h"

using namespace incre;

namespace {
#define ClearCompressHead(name) Ty _clearCompress(Ty ## name* type, const Ty& _type)
#define ClearCompressCase(name) return _clearCompress(dynamic_cast<Ty ## name*>(type.get()), type)

    ClearCompressHead(Arrow) {
        auto source = clearCompress(type->source), target = clearCompress(type->target);
        return std::make_shared<TyArrow>(source, target);
    }

    ClearCompressHead(Tuple) {
        TyList fields;
        for (auto& field: type->fields) {
            fields.push_back(clearCompress(field));
        }
        return std::make_shared<TyTuple>(fields);
    }

    ClearCompressHead(Compress) {
        return clearCompress(type->content);
    }

    ClearCompressHead(Inductive) {
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (auto& [cons_name, cons_type]: type->constructors) {
            cons_list.emplace_back(cons_name, clearCompress(cons_type));
        }
        return std::make_shared<TyInductive>(type->name, cons_list);
    }

}

Ty incre::clearCompress(const Ty &type) {
    switch (type->getType()) {
        case TyType::BOOL:
        case TyType::INT:
        case TyType::UNIT:
        case TyType::VAR: return type;
        case TyType::ARROW: ClearCompressCase(Arrow);
        case TyType::TUPLE: ClearCompressCase(Tuple);
        case TyType::COMPRESS: ClearCompressCase(Compress);
        case TyType::IND: ClearCompressCase(Inductive);
    }
}