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
bool TIncreInductive::equal(Type *t) {
    auto* it = dynamic_cast<TIncreInductive*>(t);
    if (!it) return false;
    auto* ctx = new TypeContext();
    auto res = incre::isTypeEqual(_type, it->_type, ctx);
    delete ctx;
    return res;
}

namespace {
#define TypeFromHead(name) PType _typeFromIncre(Ty ## name* type)
#define TypeFromCase(name) return _typeFromIncre(dynamic_cast<Ty ## name*>(type.get()))

    class _TCompress: public Type {
    public:
        PType content;
        _TCompress(const PType& _content): content(_content) {}
        virtual std::string getName() {
            return "compress(" + content->getName() + ")";
        }
        virtual bool equal(Type* type) {
            auto* tc = dynamic_cast<_TCompress*>(content.get());
            return tc && type::equal(content, tc->content);
        }
        virtual std::string getBaseName() {
            LOG(FATAL) << "_TCompress.getBaseName() should not be invoked";
        }
        virtual TypeList getParams() {
            LOG(FATAL) << "_TCompress.getParams() should not be invoked";
        }
        virtual PType clone(const TypeList& params) {
            return std::make_shared<_TCompress>(params[0]);
        }

    };

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
    TypeFromHead(Compress) {
        auto content = typeFromIncre(type->content);
        return std::make_shared<_TCompress>(content);
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
        case TyType::COMPRESS: TypeFromCase(Compress);
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