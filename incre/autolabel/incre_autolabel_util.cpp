//
// Created by pro on 2024/3/26.
//

#include "istool/incre/autolabel/incre_autolabel.h"

using namespace incre;
using namespace incre::syntax;
using namespace incre::autolabel;

std::pair<z3::expr, syntax::Ty> util::unfoldCompressType(syntax::TyCompress *type, z3::context &ctx) {
    auto* zc = dynamic_cast<TyZ3LabeledCompress*>(type);
    if (zc) return {zc->label, zc->body};
    return {ctx.bool_val(true), type->body};
}

std::pair<z3::expr, syntax::Ty> util::unfoldCompressType(const syntax::Ty &type, z3::context& ctx) {
    if (type->getType() != TypeType::COMPRESS) return {ctx.bool_val(false), type};
    return unfoldCompressType(dynamic_cast<TyCompress*>(type.get()), ctx);
}