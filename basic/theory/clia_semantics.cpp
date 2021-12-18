//
// Created by pro on 2021/12/6.
//

#include "basic/basic/config.h"
#include "clia_semantics.h"
#include "glog/logging.h"
#include <map>

#define TINT type::getTInt()
#define TBOOL type::getTBool()
#define TVARA type::getTVarA()

namespace {
    int getInt(const Data &w) {
        auto *iv = dynamic_cast<IntValue *>(w.value.get());
#ifdef DEBUG
        if (!iv) {
            LOG(FATAL) << "Expect int, get " << w.toString();
        }
#endif
        return iv->w;
    }

    int getBool(const Data &w) {
        auto *bv = dynamic_cast<BoolValue *>(w.value.get());
#ifdef DEBUG
        if (!bv) {
            LOG(FATAL) << "Expect bool, get " << w.toString();
        }
#endif
        return bv->w;
    }
}

IntPlusSemantics::IntPlusSemantics() : NormalSemantics("+", TINT, {TINT, TINT}) {
}
Data IntPlusSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int w = getInt(inp_list[0]) + getInt(inp_list[1]);
    if (std::abs(w) > config::KIntINF) throw SemanticsError();
    return Data(std::make_shared<IntValue>(w));
}

IntMinusSemantics::IntMinusSemantics() : NormalSemantics("-", TINT, {TINT, TINT}) {
}
Data IntMinusSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int w = getInt(inp_list[0]) - getInt(inp_list[1]);
    if (std::abs(w) > config::KIntINF) throw SemanticsError();
    return Data(std::make_shared<IntValue>(w));
}

IntTimesSemantics::IntTimesSemantics() : NormalSemantics("*", TINT, {TINT, TINT}) {
}
Data IntTimesSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    long long w = 1ll * getInt(inp_list[0]) * getInt(inp_list[1]);
    if (std::abs(w) > config::KIntINF) throw SemanticsError();
    return Data(std::make_shared<IntValue>(w));
}

IntDivSemantics::IntDivSemantics() : NormalSemantics("div", TINT, {TINT, TINT}) {
}
Data IntDivSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int x = getInt(inp_list[0]), y = getInt(inp_list[1]);
    if (y == 0) throw SemanticsError();
    return Data(std::make_shared<IntValue>(x / y));
}

IntModSemantics::IntModSemantics() : NormalSemantics("mod", TINT, {TINT, TINT}) {
}
Data IntModSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int x = getInt(inp_list[0]), y = getInt(inp_list[1]);
    if (y == 0) throw SemanticsError();
    return Data(std::make_shared<IntValue>(x % y));
}

IntLqSemantics::IntLqSemantics() : NormalSemantics("<", TBOOL, {TINT, TINT}) {
}
Data IntLqSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int x = getInt(inp_list[0]), y = getInt(inp_list[1]);
    return Data(std::make_shared<BoolValue>(x < y));
}

IntGqSemantics::IntGqSemantics() : NormalSemantics(">", TBOOL, {TINT, TINT}) {
}
Data IntGqSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int x = getInt(inp_list[0]), y = getInt(inp_list[1]);
    return Data(std::make_shared<BoolValue>(x > y));
}

IntLeqSemantics::IntLeqSemantics() : NormalSemantics("<=", TBOOL, {TINT, TINT}) {
}
Data IntLeqSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int x = getInt(inp_list[0]), y = getInt(inp_list[1]);
    return Data(std::make_shared<BoolValue>(x <= y));
}

IntGeqSemantics::IntGeqSemantics() : NormalSemantics(">=", TBOOL, {TINT, TINT}) {
}
Data IntGeqSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    int x = getInt(inp_list[0]), y = getInt(inp_list[1]);
    return Data(std::make_shared<BoolValue>(x >= y));
}

EqSemantics::EqSemantics(): NormalSemantics("=", TBOOL, {TVARA, TVARA}) {
}
Data EqSemantics::run(DataList &&inp_list, ExecuteInfo* info) {
    return Data(std::make_shared<BoolValue>(inp_list[0] == inp_list[1]));
}

NotSemantics::NotSemantics() : NormalSemantics("not", TBOOL, {TBOOL}) {
}
Data NotSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    bool b = getBool(inp_list[0]);
    return Data(std::make_shared<BoolValue>(!b));
}

AndSemantics::AndSemantics() : Semantics("and"), TypedSemantics(TBOOL, {TBOOL, TBOOL}) {
}
Data AndSemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    bool b = getBool(sub_list[0]->run(info));
    if (!b) return Data(std::make_shared<BoolValue>(false));
    return sub_list[1]->run(info);
}

OrSemantics::OrSemantics() : Semantics("or"), TypedSemantics(TBOOL, {TBOOL, TBOOL}) {
}
Data OrSemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    bool b = getBool(sub_list[0]->run(info));
    if (b) return Data(std::make_shared<BoolValue>(true));
    return sub_list[1]->run(info);
}

ImplySemantics::ImplySemantics(): Semantics("=>"), TypedSemantics(TBOOL, {TBOOL, TBOOL}) {
}
Data ImplySemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    bool b = getBool(sub_list[0]->run(info));
    if (!b) return Data(std::make_shared<BoolValue>(true));
    return sub_list[1]->run(info);
}

IteSemantics::IteSemantics() : Semantics("ite"), TypedSemantics(TVARA, {TBOOL, TVARA, TVARA}) {
}
Data IteSemantics::run(const ProgramList &sub_list, ExecuteInfo *info) {
    bool b = getBool(sub_list[0]->run(info));
    if (b) return sub_list[1]->run(info);
    return sub_list[2]->run(info);
}

#define RIG(x, y) semantics::registerSemantics(x, std::make_shared<y ## Semantics>())

VTAssigner clia::loadTheoryCLIA() {
    RIG("+", IntPlus); RIG("-", IntMinus); RIG("*", IntTimes); RIG("div", IntDiv);
    RIG("mod", IntMod); RIG("<", IntLq); RIG(">", IntGq); RIG("<=", IntLeq);
    RIG(">=", IntGeq); RIG("=", Eq); RIG("and", And); RIG("or", Or);
    RIG("not", Not); RIG("=>", Imply); RIG("ite", Ite);
    return [](const Data& data) -> PType{
        if (dynamic_cast<IntValue*>(data.get())) return std::make_shared<TInt>();
        if (dynamic_cast<BoolValue*>(data.get())) return std::make_shared<TBool>();
        LOG(FATAL) << "Unknown data for CLIA " << data.toString();
    };
}