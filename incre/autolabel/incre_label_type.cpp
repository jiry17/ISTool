//
// Created by pro on 2022/12/6.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;


#define LabelTypeHead(name) Ty _labelType(Ty ## name* type, const Ty& _type, LabelContext* ctx, std::vector<std::string>& tmp_names)
#define LabelTypeCase(name) return _labelType(dynamic_cast<Ty ## name*>(type.get()), type, ctx, tmp_names)

namespace {
    Ty _labelType(const Ty& type, LabelContext* ctx, std::vector<std::string>& tmp_names);

    bool _lookup(const std::string& name, const std::vector<std::string>& tmp_names) {
        for (const auto& tmp_name: tmp_names) {
            if (name == tmp_name) return true;
        }
        return false;
    }

    LabelTypeHead(Var) {
        if (_lookup(type->name, tmp_names)) return _type;
        auto full_type = ctx->lookup(type->name);
        if (isLabeledType(full_type.get())) {
            auto label = ctx->getCompressVar(type);
            // "Compress Compress T" is invalid
            auto head = getLabel(full_type.get());
            ctx->addConstraint(!(head && label));

            auto* ti = dynamic_cast<TyLabeledInductive*>(full_type.get());
            if (ti) return std::make_shared<TyLabeledInductive>(label || head, ti->name, ti->constructors);
            auto* tt = dynamic_cast<TyLabeledTuple*>(full_type.get());
            if (tt) return std::make_shared<TyLabeledTuple>(label || head, tt->fields);
            LOG(FATAL) << "Unexpcted labeled type " << type->toString();
        }
        return full_type;
    }

    LabelTypeHead(Tuple) {
        TyList fields;
        for (auto& field: type->fields) fields.push_back(_labelType(field, ctx, tmp_names));
        if (isCompressibleType(_type, ctx)) {
            auto label = ctx->getCompressVar(type);
            return std::make_shared<TyLabeledTuple>(label, fields);
        }
        return std::make_shared<TyLabeledTuple>(ctx->ctx.bool_val(false), fields);
    }

    LabelTypeHead(Compress) {
        auto content = _labelType(type->content, ctx, tmp_names);
        if (!isLabeledType(content.get())) {
            LOG(FATAL) << "Unexpected initial label: type " << content->toString() << " is not compressible";
        }
        auto head = getLabel(content.get());
        ctx->addConstraint(head);
        return content;
    }

    LabelTypeHead(Inductive) {
        tmp_names.push_back(type->name);
        auto log = ctx->bind(type->name, unfoldBasicType(_type, ctx));
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (auto& [cons_name, cons_type]: type->constructors) {
            cons_list.emplace_back(cons_name, _labelType(cons_type, ctx, tmp_names));
        }
        ctx->cancelBind(log);

        tmp_names.pop_back();
        if (isCompressibleType(_type, ctx)) {
            auto label = ctx->getCompressVar(type);
            return std::make_shared<TyLabeledInductive>(label, type->name, cons_list);
        }
        return std::make_shared<TyLabeledInductive>(ctx->ctx.bool_val(false), type->name, cons_list);
    }

    LabelTypeHead(Arrow) {
        auto source = _labelType(type->source, ctx, tmp_names);
        auto target = _labelType(type->target, ctx, tmp_names);
        return std::make_shared<TyArrow>(source, target);
    }

    Ty _labelType(const Ty& type, LabelContext* ctx, std::vector<std::string>& tmp_names) {
        switch (type->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
                return type;
            case TyType::VAR:
                LabelTypeCase(Var);
            case TyType::TUPLE:
                LabelTypeCase(Tuple);
            case TyType::COMPRESS:
                LabelTypeCase(Compress);
            case TyType::IND:
                LabelTypeCase(Inductive);
            case TyType::ARROW:
                LabelTypeCase(Arrow);
        }
    }
}

Ty autolabel::labelType(const Ty &type, LabelContext *ctx) {
    std::vector<std::string> tmp_names;
    auto res = _labelType(type, ctx, tmp_names);
    LOG(INFO) << "label type " << type->toString() << " || " << res->toString();
    return res;
}