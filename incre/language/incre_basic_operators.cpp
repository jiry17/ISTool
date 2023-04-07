//
// Created by pro on 2022/9/18.
//

#include "istool/incre/language/incre_value.h"
#include "istool/incre/language/incre_term.h"
#include "glog/logging.h"

using namespace incre;

namespace {
    int getInt(const Data& data) {
        return theory::clia::getIntValue(data);
    }
    bool getBool(const Data& data) {
        return data.isTrue();
    }

#define Wrap(opname, param_num, sem, ty) Data(std::make_shared<VOpFunction>(opname, param_num, sem, ty))
#define WrapTerm(opname, param_num, sem, ty) std::make_shared<TmValue>(Wrap(opname, param_num, sem, ty))
#define BuildArrow(in, out) std::make_shared<TyArrow>(std::make_shared<Ty ## in>(), std::make_shared<Ty ## out>())
#define BuildBinaryOp(in1, in2, out) std::make_shared<TyArrow>(std::make_shared<Ty ## in1>(), BuildArrow(in2, out))
#define BinaryOperator(name, itype, rtype, exp, opname, ty) \
    Term build ## name() { \
    auto func = [](const DataList& inp) -> Data {           \
        auto x = get ## itype(inp[0]);                      \
        auto y = get ## itype(inp[1]);                      \
        return BuildData(rtype, exp); \
    }; \
    return WrapTerm(opname, 2, func, ty); \
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
        auto func = [](const DataList& inps) {
            int x = getBool(inps[0]);
            return Data(std::make_shared<VBool>(!x));
        };
        return std::make_shared<TmValue>(Data(std::make_shared<VOpFunction>("not", 1, func,
                std::make_shared<TyArrow>(
                std::make_shared<TyBool>(),std::make_shared<TyBool>()))));
    }

    /*const static std::unordered_map<std::string, Term> basic_operator_map = {
            {"+", buildPlus()}, {"-", buildMinus()}, {"*", buildTimes()},
            {"/", buildDiv()}, {"==", buildEq()}, {"<", buildLt()}, {">", buildGt()},
            {"and", buildAnd()}, {"or", buildOr()}, {"=", buildEq()}, {"not", buildNot()}
    };*/

    std::unordered_map<std::string, Term> KBasicOperatorMap;

    void _initOperatorMap() {
        if (!KBasicOperatorMap.empty()) return;
        KBasicOperatorMap = {
                {"+", buildPlus()}, {"-", buildMinus()}, {"*", buildTimes()},
                {"/", buildDiv()}, {"==", buildEq()}, {"<", buildLt()}, {">", buildGt()},
                {"and", buildAnd()}, {"or", buildOr()}, {"=", buildEq()}, {"not", buildNot()}
        };
    }
}

Term incre::getOperator(const std::string &name) {
    _initOperatorMap();
    auto it = KBasicOperatorMap.find(name);
    if (it == KBasicOperatorMap.end()) {
        LOG(FATAL) << "Unknown operator " << name;
    }
    return it->second;
}
bool incre::isBasicOperator(const std::string &name) {
    return KBasicOperatorMap.find(name) != KBasicOperatorMap.end();
}