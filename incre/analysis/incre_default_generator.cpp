//
// Created by pro on 2022/9/24.
//

#include "istool/incre/analysis/incre_instru_runtime.h"
#include "glog/logging.h"
#include <stack>

using namespace incre;

namespace {
    const int KSizeLimit = 10;
    const int KTermSizeLimit = 3;
    const int KIntMin = -5;
    const int KIntMax = 5;

    // TODO: Deal with nested inductive type

    int _getSize(TyData* ty) {
        switch (ty->getType()) {
            case TyType::VAR:
            case TyType::IND: return 1;
            case TyType::ARROW: assert(0);
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT: return 0;
            case TyType::COMPRESS: {
                auto* tc = dynamic_cast<TyCompress*>(ty);
                return _getSize(tc->content.get());
            }
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(ty);
                int res = 0;
                for (const auto& field: tt->fields) res += _getSize(field.get());
                return res;
            }
        }
    }
    bool _isSelfRecursion(TyData* ty, const std::string& name) {
        switch (ty->getType()) {
            case TyType::VAR: {
                auto* tvar = dynamic_cast<TyVar*>(ty); return tvar->name == name;
            }
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
            case TyType::IND: return false;
            case TyType::COMPRESS: {
                auto* tc = dynamic_cast<TyCompress*>(ty);
                return _isSelfRecursion(tc->content.get(), name);
            }
            case TyType::ARROW: assert(0);
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(ty);
                for (const auto& field: tt->fields) {
                    if (_isSelfRecursion(field.get(), name)) return true;
                }
                return false;
            }
        }
    }

    std::vector<int> _distributeSize(int tot, int num, std::minstd_rand* e) {
        std::vector<int> res(num, 0);
        auto d = std::uniform_int_distribution<int>(0, num - 1);
        for (int i = 0; i < tot; ++i) res[d(*e)]++;
        return res;
    }

    struct RandomContext {
        std::minstd_rand* e;
        std::stack<int> size;
        std::vector<std::pair<std::string, Ty>> ctx;
        RandomContext(std::minstd_rand* _e): e(_e) {}
        Ty lookup(const std::string& name) {
            for (int i = int(ctx.size()); i; --i) {
                auto& [tname, type] = ctx[i - 1];
                if (tname == name) return type;
            }
            assert(0);
        }
        void clear() {
            while (!size.empty()) size.pop();
        }
    };

    Data _getRandomData(const Ty& type, RandomContext* ctx, SizeSafeValueGenerator* g);

#define RandomCase(name) return _getRandomData(dynamic_cast<Ty ## name*>(type.get()), type, ctx, g)
#define RandomHead(name) Data _getRandomData(Ty ## name* type, const Ty& _type, RandomContext* ctx, SizeSafeValueGenerator* g)

    RandomHead(Int) {
        auto d = std::uniform_int_distribution<int>(KIntMin, KIntMax);
        return BuildData(Int, d(*(ctx->e)));
    }
    RandomHead(Bool) {
        auto d = std::bernoulli_distribution();
        return BuildData(Bool, d(*(ctx->e)));
    }
    RandomHead(Unit) {
        return Data(std::make_shared<VUnit>());
    }
    RandomHead(Tuple) {
        DataList fields;
        for (auto& sub: type->fields) fields.push_back(_getRandomData(sub, ctx, g));
        return BuildData(Product, fields);
    }
    RandomHead(Var) {
        return _getRandomData(ctx->lookup(type->name), ctx, g);
    }
    RandomHead(Inductive) {
        auto size = ctx->size.top(); ctx->size.pop();
        ctx->ctx.emplace_back(type->name, _type);
        auto cases = g->getPossibleSplit(_type, size);
        if (cases->empty()) throw SemanticsError();
        std::uniform_int_distribution<int> d(0, int(cases->size()) - 1);
        auto selected_case = cases->at(d(*ctx->e));
        for (int i = int(selected_case.size_list.size()) - 1; i >= 0; --i) {
            ctx->size.push(selected_case.size_list[i]);
        }
        auto sub_res = _getRandomData(selected_case.cons_type, ctx, g);
        ctx->ctx.pop_back();
        auto iv = std::make_shared<VInductive>(selected_case.cons_name, sub_res);
        return Data(iv);
    }

    Data _getRandomData(const Ty& type, RandomContext* ctx, SizeSafeValueGenerator* g) {
        switch (type->getType()) {
            case TyType::INT: RandomCase(Int);
            case TyType::BOOL: RandomCase(Bool);
            case TyType::UNIT: RandomCase(Unit);
            case TyType::COMPRESS:
            case TyType::ARROW:
                assert(0);
            case TyType::TUPLE: RandomCase(Tuple);
            case TyType::VAR: RandomCase(Var);
            case TyType::IND: RandomCase(Inductive);
        }
    }
}

IncreDataGenerator::IncreDataGenerator(Env *_env): env(_env) {
}

SizeSafeValueGenerator::SplitScheme::SplitScheme(const std::string &_cons_name, const Ty &_cons_type,
                                                 const std::vector<int> &_size_list):
                                                 cons_name(_cons_name), cons_type(_cons_type), size_list(_size_list) {
}

std::string SizeSafeValueGenerator::SplitScheme::toString() const {
    std::string res = cons_name + "@" + cons_type->toString();
    for (auto& w: size_list) res += " " + std::to_string(w);
    return res;
}
SizeSafeValueGenerator::SizeSafeValueGenerator(Env* _env): IncreDataGenerator(_env) {
}
SizeSafeValueGenerator::~SizeSafeValueGenerator() noexcept {
    for (auto& [name, l]: split_map) delete l;
}

namespace {
    void _collectSubInductive(const Ty& type, TyList& sub_list,
                              std::unordered_map<std::string, Ty>& known_type) {
        if (type->getType() == TyType::IND) {
            sub_list.push_back(type);
            auto* ti = dynamic_cast<TyInductive*>(type.get());
            known_type[ti->name] = type;
            return;
        }
        if (type->getType() == TyType::VAR) {
            auto* tv = dynamic_cast<TyVar*>(type.get());
            sub_list.push_back(known_type[tv->name]);
            return;
        }
        if (type->getType() == TyType::TUPLE) {
            auto* tt = dynamic_cast<TyTuple*>(type.get());
            for (int i = 0; i < tt->fields.size(); ++i) {
                _collectSubInductive(tt->fields[i], sub_list, known_type);
            }
            return;
        }
    }

    void _combineAll(int pos, int rem, const std::vector<std::vector<int>>& size_pool, std::vector<int>& sub_list,
                     std::vector<std::vector<int>>& res) {
        if (pos == size_pool.size()) {
            if (rem == 0) res.push_back(sub_list);
            return;
        }
        for (auto now: size_pool[pos]) {
            if (now <= rem) {
                sub_list[pos] = now;
                _combineAll(pos + 1, rem - now, size_pool, sub_list, res);
            }
        }
    }

    std::vector<std::vector<int>> _combineAll(const std::vector<std::vector<int>>& size_pool, int target) {
        std::vector<std::vector<int>> res;
        std::vector<int> sub_list(size_pool.size());
        _combineAll(0, target, size_pool, sub_list, res);
        return res;
    }
}

SizeSafeValueGenerator::SplitList *SizeSafeValueGenerator::getPossibleSplit(const Ty& _type, int size) {
    auto* type = dynamic_cast<TyInductive*>(_type.get());
    if (!type) LOG(INFO) << "Expected TyInductive, but get " << type->toString();
    auto feature = type->name + "@" + std::to_string(size);
    if (split_map.count(feature)) return split_map[feature];
    if (!size) return split_map[feature] = new SplitList();
    auto* res = new SplitList();
    for (auto& [cons_name, cons_type]: type->constructors) {
        TyList sub_list;
        _collectSubInductive(cons_type, sub_list, ind_map);
        /*LOG(INFO) << "case for " << cons_name << " " << cons_type->toString() << " " << sub_list.size();
        for (auto& sub_type: sub_list) {
            LOG(INFO) << "  " << sub_type->toString();
        }*/

        std::vector<std::vector<int>> merge_list;
        for (const auto& sub: sub_list) {
            std::vector<int> current_valid;
            for (int i = 1; i < size; ++i) {
                //LOG(INFO) << "get for " << sub->toString() << " " << i << " " << getPossibleSplit(sub, i);
                if (!(getPossibleSplit(sub, i)->empty())) {
                    current_valid.push_back(i);
                }
            }
            merge_list.push_back(current_valid);
        }

        for (auto& valid_sub_list: _combineAll(merge_list, size - 1)) {
            res->emplace_back(cons_name, cons_type, valid_sub_list);
        }
    }
    /*LOG(INFO) << "Possible cases for " << feature;
    for (auto& possible_case: *res) {
        LOG(INFO) << "  " << possible_case.toString();
    }*/
    return split_map[feature] = res;
}

#include <iostream>

Data SizeSafeValueGenerator::getRandomData(const Ty& type) {
    auto* ctx = new RandomContext(&env->random_engine);
    TyList sub_list;
    _collectSubInductive(type, sub_list, ind_map);
    // LOG(INFO) << "target type " << type->toString();
    if (sub_list.empty()) {
        auto res = _getRandomData(type, ctx, nullptr);
        delete ctx;
        return res;
    }
    auto d = std::uniform_int_distribution<int>(int(sub_list.size()), KSizeLimit);
    std::vector<std::vector<int>> size_pool;
    for (auto sub: sub_list) {
        std::vector<int> current_valid;
        for (int i = 1; i <= KSizeLimit; ++i) {
            if (!getPossibleSplit(sub, i)->empty()) {
                current_valid.push_back(i);
            }
        }
        size_pool.push_back(current_valid);
    }
    /*LOG(INFO) << "Generator for " << type->toString();
    for (int i = 0; i < sub_list.size(); ++i) {
        std::cout << sub_list[i]->toString();
        for (auto j: size_pool[i]) std::cout << " " << j;
        std::cout << std::endl;
    }*/
    Data res;

    while (1) {
        try {
            ctx->clear();
            int start_size = d(env->random_engine);
            auto cases = _combineAll(size_pool, start_size);
            if (cases.empty()) continue;
            auto d2 = std::uniform_int_distribution<int>(0, int(cases.size()) - 1);
            auto& selected_case = cases[d2(env->random_engine)];
            for (int i = int(selected_case.size()) - 1; i >= 0; --i) {
                ctx->size.push(selected_case[i]);
            }
            res = _getRandomData(type, ctx, this);
            break;
        } catch (SemanticsError& e) {
            continue;
        }
    }

    delete ctx;
    return res;
}



FirstOrderFunctionGenerator::FirstOrderFunctionGenerator(Env *_env): SizeSafeValueGenerator(_env) {
}

namespace {
    struct ComponentInfo {
    public:
        TyList inp_list;
        Ty oup;
        Term op;
        ComponentInfo(const TyList& _inp_list, const Ty& _oup, const Term& _op):
          inp_list(_inp_list), oup(_oup), op(_op) {
        }
    };

    Term _generateTerm(int term_size, const std::vector<ComponentInfo>& component_list, const Ty& target, std::minstd_rand* e) {
        std::vector<int> possible_component;
        for (int i = 0; i < component_list.size(); ++i) {
            if (!isTypeEqual(component_list[i].oup, target, nullptr)) continue;
            if (term_size) {
                if (component_list[i].inp_list.size()) possible_component.push_back(i);
            } else if (component_list[i].inp_list.empty()) possible_component.push_back(i);
        }
        if (possible_component.empty()) return nullptr;
        std::uniform_int_distribution<int> d(0, int(possible_component.size()) - 1);
        auto& component = component_list[possible_component[d(*e)]];
        Term res = component.op;
        auto sub_size_list = _distributeSize(term_size - 1, component.inp_list.size(), e);
        for (int i = 0; i < component.inp_list.size(); ++i) {
            auto sub = _generateTerm(sub_size_list[i], component_list, component.inp_list[i], e);
            if (!sub) return nullptr;
            res = std::make_shared<TmApp>(res, sub);
        }
        return res;
    }
}

Data FirstOrderFunctionGenerator::getRandomData(const Ty& type) {
    if (type->getType() != TyType::ARROW) return SizeSafeValueGenerator::getRandomData(type);

    std::vector<ComponentInfo> component_list;
    Ty oup_type;
    TyList param_list;
    Ty current_type = type;
    while (type->getType() == TyType::ARROW) {
        auto* at = dynamic_cast<TyArrow*>(current_type.get());
        current_type = at->target; oup_type = at->target;
        auto param_name = "v" + std::to_string(param_list.size());
        param_list.push_back(at->source);
        component_list.emplace_back((TyList){}, at->source, std::make_shared<TmVar>(param_name));
    }

    auto op = getOperator("+"); auto ti = std::make_shared<TyInt>();
    component_list.emplace_back((TyList){ti, ti}, ti, op);
    component_list.emplace_back((TyList){}, ti, std::make_shared<TmValue>(BuildData(Int, 1)));

    std::uniform_int_distribution<int> dist(0, KTermSizeLimit);

    while (true) {
        int term_size = dist(env->random_engine);
        auto res = _generateTerm(term_size, component_list, oup_type, &env->random_engine);
        if (res) {
            for (int i = int(param_list.size()) - 1; i >= 0; --i) {
                auto param_name = "v" + std::to_string(i);
                res = std::make_shared<TmAbs>(param_name, param_list[i], res);
            }
            // LOG(INFO) << "generate term " << res->toString();
            return run(res, nullptr);
        }
    }
}

namespace {
#define X std::make_shared<TmVar>("x")
#define Y std::make_shared<TmVar>("y")
#define APP(t1, t2) std::make_shared<TmApp>(t1, t2)
#define ITE(c, t, f) std::make_shared<TmIf>(c, t, f)
#define OP(op, t1, t2) APP(APP(incre::getOperator(op), t1), t2)
#define IVAL(i) std::make_shared<TmValue>(BuildData(Int, i))
#define BuildIntUnary(name, term) std::make_shared<TmAbs>(name, std::make_shared<TyInt>(), term)
#define BuildIntBinary(term) BuildIntUnary("x", BuildIntUnary("y", term))
#define TI std::make_shared<TyInt>()
#define TARROW(s, t) std::make_shared<TyArrow>(s, t)

    TermList KBinaryIntOperator, KUnaryIntOperator;
    std::vector<std::pair<Ty, TermList>> KOperatorPool;

    void _initOperatorPool() {
        if (!KBinaryIntOperator.empty()) return;
        KBinaryIntOperator = {
                BuildIntBinary(X), BuildIntBinary(Y), BuildIntBinary(OP("+", X, Y)),
                BuildIntBinary(ITE(OP("<", X, Y), X, Y)), BuildIntBinary(ITE(OP("<", Y, X), X, Y)),
                BuildIntBinary(OP("+", X, IVAL(1))), BuildIntBinary(OP("+", IVAL(1), Y))
        };
        KUnaryIntOperator = {
                BuildIntUnary("x", X), BuildIntUnary("x", OP("+", X, IVAL(1))), BuildIntUnary("x", IVAL(1))
        };
        KOperatorPool = {
                {TARROW(TI, TI), KUnaryIntOperator}, {TARROW(TI, TARROW(TI, TI)), KBinaryIntOperator}
        };
    }
}

FixedPoolFunctionGenerator::FixedPoolFunctionGenerator(Env *_env): SizeSafeValueGenerator(_env) {
}

Data FixedPoolFunctionGenerator::getRandomData(const Ty& type) {
    _initOperatorPool();
    for (auto& [ty, pool]: KOperatorPool) {
        if (type->toString() == ty->toString()) {
            std::uniform_int_distribution<int> d(0, int(pool.size()) - 1);
            return Data(std::make_shared<VAbsFunction>(pool[d(env->random_engine)]));
        }
    }
    return SizeSafeValueGenerator::getRandomData(type);
}