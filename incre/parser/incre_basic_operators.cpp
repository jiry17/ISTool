//
// Created by pro on 2022/9/18.
//

#include "istool/incre/parser/incre_from_json.h"
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

#define Wrap(f, opname, ty) Data(std::make_shared<VBasicOperator>(f, opname, ty))
#define WrapTerm(f, opname, ty) std::make_shared<TmValue>(Wrap(f, opname, ty))
#define BuildArrow(in, out) std::make_shared<TyArrow>(std::make_shared<Ty ## in>(), std::make_shared<Ty ## out>())
#define BuildBinaryOp(in1, in2, out) std::make_shared<TyArrow>(std::make_shared<Ty ## in1>(), BuildArrow(in2, out))
#define BinaryOperator(name, rtype, exp, opname, ty) \
    Term build ## name() { \
    auto func = [](const Term& term) { \
        int x = term2Int(term); \
        auto func = [x](const Term& term) { \
            int y = term2Int(term); \
            return BuildData(rtype, exp); \
        }; \
        return Data(std::make_shared<VFunction>(func)); \
    }; \
    return WrapTerm(func, opname, ty); \
}

    BinaryOperator(Plus, Int, x + y, "+", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Minus, Int, x - y, "-", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Times, Int, x * y, "*", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Div, Int, x / y, "/", BuildBinaryOp(Int, Int, Int))
    BinaryOperator(Eq, Bool, x == y, "==", BuildBinaryOp(Int, Int, Bool))
    BinaryOperator(Lt, Bool, x < y, "<", BuildBinaryOp(Int, Int, Bool))
    BinaryOperator(Gt, Bool, x > y, ">", BuildBinaryOp(Int, Int, Bool))

    const static std::unordered_map<std::string, Term> basic_operator_map = {
            {"+", buildPlus()}, {"-", buildMinus()}, {"*", buildTimes()},
            {"/", buildDiv()}, {"==", buildEq()}, {"<", buildLt()}, {">", buildGt()}
    };
}

Term incre::getOperator(const std::string &name) {
    auto it = basic_operator_map.find(name);
    if (it == basic_operator_map.end()) {
        LOG(FATAL) << "Unknown operator " << name;
    }
    return it->second;
}