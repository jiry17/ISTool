//
// Created by pro on 2022/9/25.
//

#include "istool/incre/trans/incre_trans.h"
#include "istool/ext/deepcoder/data_type.h"
#include "glog/logging.h"

using namespace incre;

// TODO: add checks for alpha-renaming
PType TIncreInductive::clone(const TypeList &params) {
    return std::make_shared<TIncreInductive>(_type);
}
std::string TIncreInductive::getName() {
    return type->toString();
}
TIncreInductive::TIncreInductive(const Ty &__type): _type(__type) {
    type = dynamic_cast<TyInductive*>(_type.get());
    assert(type);
}

namespace {
#define TypeFromHead(name) PType _typeFromIncre(Ty ## name* type)
#define TypeFromCase(name) return _typeFromIncre(dynamic_cast<Ty ## name*>(type.get()))

    TypeFromHead(Int) {
        return theory::clia::getTInt();
    }
    TypeFromHead(Unit) {
        return std::make_shared<TBot>();
    }
    TypeFromHead(Bool) {
        return type::getTBool();
    }
    TypeFromHead(Tuple) {
        TypeList components;
        for (const auto& field: type->fields) {
            components.push_back(typeFromIncre(field));
        }
        return std::make_shared<TProduct>(components);
    }
    TypeFromHead(Arrow) {
        auto source = typeFromIncre(type->source);
        auto target = typeFromIncre(type->target);
        return std::make_shared<TArrow>((TypeList){source}, target);
    }
}

PType incre::typeFromIncre(const Ty& type) {
    switch (type->getType()) {
        case TyType::INT: TypeFromCase(Int);
        case TyType::UNIT: TypeFromCase(Unit);
        case TyType::BOOL: TypeFromCase(Bool);
        case TyType::TUPLE: TypeFromCase(Tuple);
        case TyType::IND: return std::make_shared<TIncreInductive>(type);
        case TyType::ARROW: TypeFromCase(Arrow);
        case TyType::COMPRESS:
        case TyType::VAR:
            LOG(FATAL) << "Unknown type for translating from Incre: " << type->toString();
    }
}

Ty incre::typeToIncre(Type *type) {
    {
        auto* it = dynamic_cast<TInt*>(type);
        if (it) return std::make_shared<TyInt>();
    }
    {
        auto* bt = dynamic_cast<TBool*>(type);
        if (bt) return std::make_shared<TyBool>();
    }
    {
        auto* bt = dynamic_cast<TBot*>(type);
        if (bt) return std::make_shared<TyUnit>();
    }
    {
        auto* pt = dynamic_cast<TProduct*>(type);
        if (pt) {
            TyList fields;
            for (const auto& sub: pt->sub_types) {
                fields.push_back(typeToIncre(sub.get()));
            }
            return std::make_shared<TyTuple>(fields);
        }
    }
    {
        auto* at = dynamic_cast<TArrow*>(type);
        if (at) {
            auto res = typeToIncre(at->oup_type.get());
            for (int i = int(at->inp_types.size()) - 1; i >= 0; --i) {
                auto inp = typeToIncre(at->inp_types[i - 1].get());
                res = std::make_shared<TyArrow>(inp, res);
            }
            return res;
        }
    }
    {
        auto* it = dynamic_cast<TIncreInductive*>(type);
        if (it) return it->_type;
    }
    LOG(FATAL) << "Unknown type for translating to Incre: " << type->getName();
}