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

#define Wrap(f, opname) Data(std::make_shared<VNamedFunction>(f, opname))
#define WrapTerm(f, opname) std::make_shared<TmValue>(Wrap(f, opname))
#define BinaryOperator(name, rtype, exp, opname) \
    Term build ## name() { \
    auto func = [](const Term& term) { \
        int x = term2Int(term); \
        auto func = [x](const Term& term) { \
            int y = term2Int(term); \
            return BuildData(rtype, exp); \
        }; \
        return Data(std::make_shared<VFunction>(func)); \
    }; \
    return WrapTerm(func, opname); \
}

    BinaryOperator(Plus, Int, x + y, "+")
    BinaryOperator(Minus, Int, x - y, "-")
    BinaryOperator(Times, Int, x * y, "*")
    BinaryOperator(Div, Int, x / y, "/")
    BinaryOperator(Eq, Bool, x == y, "==")
    BinaryOperator(Lt, Bool, x < y, "<")
    BinaryOperator(Gt, Bool, x > y, ">")

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