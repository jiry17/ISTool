//
// Created by pro on 2022/2/2.
//

#include "istool/dsl/samplesy/samplesy_dsl.h"
#include "istool/dsl/samplesy/samplesy_witness.h"
#include "istool/dsl/samplesy/samplesy_semantics.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/basic/string/string_semantics.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/basic/clia/clia_semantics.h"
#include "istool/sygus/theory/theory.h"
#include "istool/sygus/theory/basic/theory_semantics.h"
#include "istool/sygus/theory/witness/theory_witness.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include "istool/ext/vsa/vsa_extension.h"
#include "glog/logging.h"
#include <unordered_set>

using theory::string::getStringValue;
#define TSTRING theory::string::getTString()
#define TINT theory::clia::getTInt()

namespace {
    bool _isValidType(Type* type) {
        return dynamic_cast<TInt*>(type) || dynamic_cast<TString*>(type);
    }

    void _checkType(Type* type) {
        if (!_isValidType(type)) LOG(FATAL) << "SampleSy: Unsupported type " << type->getName();
    }
}

namespace {
    const std::string KSampleSyIndexMaxName = "SampleSy@IndexMax";
    const int KDefaultIndexMax = 20;
}

Grammar * samplesy::getSampleSyGrammar(const TypeList &inp_types, const PType &oup_type, const DataList &const_list, Env* env) {
    for (const auto& type: inp_types) _checkType(type.get());
    _checkType(oup_type.get());

    auto* sc_symbol = new NonTerminal("sc", TSTRING);
    auto* p_symbol = new NonTerminal("p", TSTRING);
    auto* ic_symbol = new NonTerminal("ic", TINT);
    auto* i_symbol = new NonTerminal("i", TINT);

    auto* s_symbol = new NonTerminal("s", TSTRING);
    auto* sub_symbol = new NonTerminal("sub", TSTRING);
    auto* ind_symbol = new NonTerminal("ind", TINT);
    for (int i = 0; i < inp_types.size(); ++i) {
        auto type = inp_types[i];
        auto* rule = new Rule(semantics::buildParamSemantics(i, type), {});
        if (dynamic_cast<TString*>(type.get())) {
            p_symbol->rule_list.push_back(rule);
        } else {
            i_symbol->rule_list.push_back(rule);
            ind_symbol->rule_list.push_back(rule);
        }
    }
    for (auto& c: const_list) {
        auto* rule = new Rule(semantics::buildConstSemantics(c), {});
        auto* sv = dynamic_cast<StringValue*>(c.get());
        if (sv && !sv->s.empty()) {
            sc_symbol->rule_list.push_back(rule);
        }
    }
    int index_max = theory::clia::getIntValue(*(env->getConstRef(KSampleSyIndexMaxName, BuildData(Int, KDefaultIndexMax))));
    for (int i = -index_max; i <= index_max;++i) {
        ic_symbol->rule_list.push_back(new Rule(semantics::buildConstSemantics(BuildData(Int, i)), {}));
    }
    s_symbol->rule_list.push_back(new Rule(env->getSemantics("str.++"), {s_symbol, s_symbol}));
    s_symbol->rule_list.push_back(new Rule(env->getSemantics("replace"), {sub_symbol, sc_symbol, sc_symbol}));
    s_symbol->rule_list.push_back(new Rule(env->getSemantics("delete"), {sub_symbol, sc_symbol}));
    s_symbol->rule_list.push_back(new Rule(env->getSemantics(""), {sub_symbol}));

    sc_symbol->rule_list.push_back(new Rule(env->getSemantics(""), {p_symbol}));

    sub_symbol->rule_list.push_back(new Rule(env->getSemantics(""), {sc_symbol}));
    sub_symbol->rule_list.push_back(new Rule(env->getSemantics("substr"), {p_symbol, i_symbol, i_symbol}));

    i_symbol->rule_list.push_back(new Rule(env->getSemantics(""), {ic_symbol}));
    i_symbol->rule_list.push_back(new Rule(env->getSemantics("move"), {ind_symbol, ic_symbol}));

    ind_symbol->rule_list.push_back(new Rule(env->getSemantics("indexof"), {p_symbol, s_symbol, i_symbol}));
    ind_symbol->rule_list.push_back(new Rule(env->getSemantics("str.len"), {p_symbol}));

    auto* start_symbol = dynamic_cast<TInt*>(oup_type.get()) ? i_symbol : s_symbol;
    return new Grammar(start_symbol, {s_symbol, sub_symbol, ind_symbol, sc_symbol, p_symbol, i_symbol, ic_symbol});
}

Grammar * samplesy::rewriteGrammar(Grammar *g, Env* env, FiniteIOExampleSpace* io_space) {
    TypeList inp_types;
    auto oup_type = g->start->type;

    for (auto* symbol: g->symbol_list) {
        for (auto* rule: symbol->rule_list) {
            auto* ps = dynamic_cast<ParamSemantics*>(rule->semantics.get());
            if (ps) {
                while (inp_types.size() <= ps->id) inp_types.emplace_back();
                inp_types[ps->id] = ps->oup_type;
            }
        }
    }
    for (int i = 0; i < inp_types.size(); ++i) {
        if (!inp_types[i]) LOG(FATAL) << "SampleSy: Unused input " << i;
    }

    DataList const_list;
    std::unordered_set<std::string> const_set;
    for (auto* symbol: g->symbol_list) {
        for (auto* rule: symbol->rule_list) {
            auto* cs = dynamic_cast<ConstSemantics*>(rule->semantics.get());
            if (cs) {
                auto feature = cs->w.toString();
                if (const_set.find(feature) == const_set.end()) {
                    const_set.insert(feature);
                    const_list.push_back(cs->w);
                }
            }
        }
    }

    /*const int KLengthLimit = 1, KOccurLimit = 2, KMaxNum = 3;
    for (int l = 1; l <= KLengthLimit; ++l) {
        std::unordered_map<std::string, int> occur_map;
        for (const auto& example: io_space->example_space) {
            auto io_example = io_space->getIOExample(example);
            std::unordered_set<std::string> occur_set;
            for (const auto& inp: io_example.first) {
                auto* sv = dynamic_cast<StringValue*>(inp.get());
                if (sv) {
                    for (int i = 0; i + l <= sv->s.length(); ++i) {
                        occur_set.insert(sv->s.substr(i, l));
                    }
                }
            }
            for (auto& s: occur_set) occur_map[s] += 1;
        }
        std::vector<std::pair<int, std::string>> possible_cons;
        for (auto& [key, w]: occur_map) {
            if (w * KOccurLimit >= io_space->example_space.size()) {
                possible_cons.emplace_back(w, key);
            }
        }

        std::sort(possible_cons.begin(), possible_cons.end(), std::greater<>());
        for (int i = 0; i < possible_cons.size() && i < KMaxNum; ++i) {
            Data d = BuildData(String, possible_cons[i].second);
            auto feature = d.toString();
            if (const_set.find(feature) == const_set.end()) {
                const_set.insert(feature);
                const_list.push_back(d);
            }
        }
    }*/
    return getSampleSyGrammar(inp_types, oup_type, const_list, env);
}

void samplesy::registerSampleSyBasic(Env *env) {
    sygus::setTheory(env, TheoryToken::STRING);
    sygus::loadSyGuSTheories(env, theory::loadBasicSemantics);
    env->setSemantics("", std::make_shared<DirectSemantics>());
    env->setSemantics("replace", std::make_shared<samplesy::StringReplaceAllSemantics>());
    env->setSemantics("delete", std::make_shared<samplesy::StringDeleteSemantics>());
    env->setSemantics("substr", std::make_shared<samplesy::StringAbsSubstrSemantics>());
    env->setSemantics("indexof", std::make_shared<samplesy::StringIndexOfSemantics>());
    env->setSemantics("move", std::make_shared<samplesy::IndexMoveSemantics>());
}

void samplesy::registerSampleSyWitness(Env *env) {
    sygus::loadSyGuSTheories(env, theory::loadWitnessFunction);
    auto* const_list = env->getConstListRef(theory::string::KStringConstList);
    auto* input_list = env->getConstListRef(theory::string::KStringInputList);
    auto* int_max = env->getConstRef(theory::clia::KWitnessIntMaxName);
    auto* ext = ext::vsa::getExtension(env);

    ext->registerWitnessFunction("replace", new samplesy::StringReplaceAllWitnessFunction(const_list, input_list));
    ext->registerWitnessFunction("delete", new samplesy::StringDeleteWitnessFunction(const_list, input_list));
    ext->registerWitnessFunction("substr", new samplesy::StringAbsSubstrWitnessFunction(input_list));
    ext->registerWitnessFunction("indexof", new samplesy::StringIndexOfWitnessFunction(input_list));
    ext->registerWitnessFunction("move", new samplesy::IndexMoveWitnessFunction(int_max));
}

void samplesy::setSampleSyIndexMax(Env *env, int w) {
    env->setConst(KSampleSyIndexMaxName, BuildData(Int, w));
}