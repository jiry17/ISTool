//
// Created by pro on 2022/9/18.
//

#include "istool/incre/language/incre_value.h"
#include "istool/incre/language/incre_term.h"
#include "glog/logging.h"

using namespace incre;

namespace {
    int term2Int(const Term& term) {
        auto* vt = dynamic_cast<TmValue*>(term.get());
        if (!vt) LOG(FATAL) << "Expected integer, but got " << term->toString();
        auto* iv = dynamic_cast<VInt*>(vt->data.get());
        if (!iv) LOG(FATAL) << "Expected integer, but got " << term->toString();
        return iv->w;
    }

    bool term2Bool(const Term& term) {
        auto* vt = dynamic_cast<TmValue*>(term.get());
        if (!vt) LOG(FATAL) << "Expected integer, but got " << term->toString();
        auto* bv = dynamic_cast<VBool*>(vt->data.get());
        if (!bv) LOG(FATAL) << "Expected integer, but got " << term->toString();
        return bv->w;
    }

#define Wrap(f, opname, ty) Data(std::make_shared<VBasicOperator>(f, opname, ty))
#define WrapTerm(f, opname, ty) std::make_shared<TmValue>(Wrap(f, opname, ty))
#define BuildArrow(in, out) std::make_shared<TyArrow>(std::make_shared<Ty ## in>(), std::make_shared<Ty ## out>())
#define BuildBinaryOp(in1, in2, out) std::make_shared<TyArrow>(std::make_shared<Ty ## in1>(), BuildArrow(in2, out))
#define BinaryOperator(name, itype, rtype, exp, opname, ty) \
    Term build ## name() { \
    auto func = [](const Term& term) { \
        int x = term2 ## itype(term); \
        auto func = [x](const Term& term) { \
            int y = term2 ## itype(term); \
            return BuildData(rtype, exp); \
        }; \
        return Data(std::make_shared<VFunction>(func)); \
    }; \
    return WrapTerm(func, opname, ty); \
}

    BinaryOperator(Plus, Int, Int, x + y, "+", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Minus, Int, Int, x - y, "-", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Times, Int, Int, x * y, "*", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Div, Int, Int, x / y, "/", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Eq, Int, Bool, x == y, "==", BuildBinaryOp(Int, Int, Bool))
    BinaryOperator(Lt, Int, Bool, x < y, "<", BuildBinaryOp(Int, Int, Bool))
    BinaryOperator(Gt, Int, Bool, x > y, ">", BuildBinaryOp(Int, Int, Bool))
    BinaryOperator(And, Bool, Bool, x && y, "and", BuildBinaryOp(Bool, Bool, Bool))
    BinaryOperator(Or, Bool, Bool, x || y, "or", BuildBinaryOp(Bool, Bool, Bool))

    Term buildNot() {
        auto func = [](const Term &term) {
            int x = term2Bool(term);
            return Data(std::make_shared<VBool>(!x));
        };
        return std::make_shared<TmValue>(Data(std::make_shared<VBasicOperator>(func, "not",
                std::make_shared<TyArrow>(
                std::make_shared<TyBool>(),std::make_shared<TyBool>()))));
    }

    const static std::unordered_map<std::string, Term> basic_operator_map = {
            {"+", buildPlus()}, {"-", buildMinus()}, {"*", buildTimes()},
            {"/", buildDiv()}, {"==", buildEq()}, {"<", buildLt()}, {">", buildGt()},
            {"and", buildAnd()}, {"or", buildOr()}, {"=", buildEq()}, {"not", buildNot()}
    };
}

Term incre::getOperator(const std::string &name) {
    auto it = basic_operator_map.find(name);
    if (it == basic_operator_map.end()) {
        LOG(FATAL) << "Unknown operator " << name;
    }
    return it->second;
}
bool incre::isBasicOperator(const std::string &name) {
    return basic_operator_map.find(name) != basic_operator_map.end();
}