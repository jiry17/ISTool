//
// Created by pro on 2022/8/2.
//

#include "istool/solver/tmp/weighted_obe.h"
#include "glog/logging.h"
#include <queue>
#include <istool/ext/composed_semantics/composed_rule.h>

WeightedOBESolver::WeightedOBESolver(Specification *_spec, Verifier *_v, ProgramChecker *_is_runnable):
    OBESolver(_spec, _v, _is_runnable) {
}

namespace {
    class CachedOBEOptimizer: public Optimizer {
    public:
        std::unordered_map<std::string, std::string> output_cache;
        ProgramChecker* is_runnable;
        std::unordered_map<std::string, ExampleList> example_pool;
        std::unordered_set<std::string> visited_set;
        Env* env;
        CachedOBEOptimizer(ProgramChecker* _is_runnable, const std::unordered_map<std::string, ExampleList>& _pool, Env* _env):
            is_runnable(_is_runnable), example_pool(_pool), env(_env) {
        }
        std::string getOutput(const std::string& name, const PProgram& p) {
            auto feature = name + "@" + p->toString();
            if (output_cache.count(feature)) {
                return output_cache[feature];
            }
            auto& example_list = example_pool[name];
            std::string res;
            for (auto& example: example_list) {
                res += env->run(p.get(), example).toString() + "@";
            }
            return output_cache[feature] = res;
        }
        virtual bool isDuplicated(const std::string& name, NonTerminal* nt, const PProgram& p) {
            if (!is_runnable->isValid(p.get()) || example_pool.find(name) == example_pool.end()) return false;
            auto feature = std::to_string(nt->id) + "@" + getOutput(name, p);
            if (visited_set.find(feature) != visited_set.end()) return true;
            visited_set.insert(feature);
            return false;
        }
        virtual void clear() {
            visited_set.clear(); output_cache.clear();
        }
        virtual ~CachedOBEOptimizer() = default;
    };

    int indexAllNT(const std::vector<PSynthInfo>& info_list) {
        int id = 0;
        for (const auto& info: info_list) {
            for (auto* symbol: info->grammar->symbol_list) {
                symbol->id = id++;
            }
        }
        return id;
    }

    void _getAllSizeScheme(int pos, int sum, const std::vector<std::vector<int>>& pool, std::vector<int>& tmp, std::vector<std::vector<int>>& res) {
        if (pos == pool.size()) {
            if (sum == 0) res.push_back(tmp);
            return;
        }
        for (auto size: pool[pos]) {
            if (size > sum) continue;
            tmp.push_back(size);
            _getAllSizeScheme(pos + 1, sum - size, pool, tmp, res);
            tmp.pop_back();
        }
    }

    std::vector<std::vector<int>> getAllSizeScheme(const std::vector<std::vector<int>>& size_pool, int sum) {
        std::vector<std::vector<int>> res;
        std::vector<int> tmp;
        _getAllSizeScheme(0, sum, size_pool, tmp, res);
        return res;
    }

    void buildAllCombination(int pos,  const ProgramStorage& pool, ProgramList& tmp, ProgramStorage& res) {
        if (pos == pool.size()) {
            res.push_back(tmp); return;
        }
        for (const auto& p: pool[pos]) {
            tmp.push_back(p);
            buildAllCombination(pos + 1, pool, tmp, res);
            tmp.pop_back();
        }
    }

    ProgramStorage merge(const std::vector<int>& id_list, int size, const std::vector<ProgramStorage>& storage_list) {
        std::vector<std::vector<int> > size_pool;
        for (int id: id_list) {
            std::vector<int> size_list;
            for (int i = 0; i <= size; ++i) {
                const auto& pl = storage_list[id][i];
                if (!pl.empty()) size_list.push_back(i);
            }
            size_pool.push_back(size_list);
        }
        ProgramStorage res;
        for (const auto& scheme: getAllSizeScheme(size_pool, size)) {
            ProgramStorage pool;
            for (int i = 0; i < id_list.size(); ++i) {
                pool.push_back(storage_list[id_list[i]][scheme[i]]);
            }
            ProgramList tmp;
            buildAllCombination(0, pool, tmp, res);
        }
        return res;
    }

    bool _isDirectRule(Rule* rule) {
        auto* cr = dynamic_cast<ConcreteRule*>(rule);
        return cr && dynamic_cast<DirectSemantics*>(cr->semantics.get());
    }

    std::vector<NonTerminal*> _getDirectOrder(Grammar* g) {
        std::unordered_map<NonTerminal*, std::vector<NonTerminal*>> edge_map;
        std::unordered_map<NonTerminal*, int> d;
        for (auto* s: g->symbol_list) {
            for (auto* r: s->rule_list) {
                if (_isDirectRule(r)) {
                    d[s]++; edge_map[r->param_list[0]].push_back(s);
                }
            }
        }
        std::vector<NonTerminal*> res;
        std::queue<NonTerminal*> Q;
        for (auto* s: g->symbol_list) if (!d[s]) Q.push(s);
        while (!Q.empty()) {
            auto* s = Q.front(); Q.pop();
            res.push_back(s);
            for (auto* t: edge_map[s]) {
                --d[t]; if (d[t] == 0) Q.push(t);
            }
        }
        if (res.size() != g->symbol_list.size()) {
            LOG(FATAL) << "Cyclic direct relation in the grammar";
        }
        return res;
    }

    int _getOperatorSize(Program* p) {
        if (dynamic_cast<ParamSemantics*>(p->semantics.get())) return 0;
        int sum = 0;
        for (const auto& sub: p->sub_list) sum += _getOperatorSize(sub.get());
        return sum + 1;
    }

    int _getWeight(Rule* r) {
        auto* cr = dynamic_cast<ConcreteRule*>(r);
        if (cr) {
            if (dynamic_cast<DirectSemantics*>(cr->semantics.get())) return 0;
            return 1;
        }
        auto* pr = dynamic_cast<ComposedRule*>(r);
        if (pr) return _getOperatorSize(pr->composed_sketch.get());
        LOG(FATAL) << "Unknown rule " << r->toString();
    }

    FunctionContext _weightedEnumerate(const std::vector<PSynthInfo> &info_list, const EnumConfig &c) {
        auto* v = c.v; auto* o = c.o; o->clear();
        static int c_num = 0;
        int n = indexAllNT(info_list);
        std::vector<ProgramStorage> storage_list(n);
        for (auto& ps: storage_list) ps.emplace_back();
        std::vector<FunctionContext> res;
        std::vector<NTList> direct_order_list;
        for (const auto& info: info_list) direct_order_list.push_back(_getDirectOrder(info->grammar));


        for (int size = 1;; ++size) {
            TimeCheck(c.guard);
            for (int pos = 0; pos < info_list.size(); ++pos) {
                auto& info = info_list[pos];
                for (auto* symbol: direct_order_list[pos]) {
                    int id = symbol->id; storage_list[id].emplace_back();
                    for (auto* rule: symbol->rule_list) {
                        if (_isDirectRule(rule)) {
                            for (const auto& p: storage_list[rule->param_list[0]->id][size]) {
                                if (!o->isDuplicated(info->name, symbol, p)) {
                                    storage_list[symbol->id][size].push_back(p);
                                }
                            }
                        } else {
                            std::vector<int> sub_id_list;
                            for (auto *sub_symbol: rule->param_list) sub_id_list.push_back(sub_symbol->id);
                            ProgramStorage tmp = merge(sub_id_list, size - _getWeight(rule), storage_list);
                            for (const auto &sub_list: tmp) {
                                TimeCheck(c.guard);
                                auto p = rule->buildProgram(sub_list);
                                if (o->isDuplicated(info->name, symbol, p))
                                    continue;
                                storage_list[id][size].push_back(p);
                            }
                        }
                    }
                }
            }
            int merge_size = int(info_list.size()) + size - 1;
            std::vector<int> sub_id_list;
            for (const auto& info: info_list) {
                auto* start = info->grammar->start;
                sub_id_list.push_back(start->id);
            }
            ProgramStorage tmp = merge(sub_id_list, merge_size, storage_list);
            for (const auto& sub_list: tmp) {
                TimeCheck(c.guard);
                FunctionContext info;
                for (int i = 0; i < info_list.size(); ++i) {
                    info[info_list[i]->name] = sub_list[i];
                }
                if (v->verify(info, nullptr)) {
                    //LOG(INFO) << "construct num " << c_num;
                    //int kk; std::cin >> kk;
                    return info;
                }
            }
        }
    }
}

FunctionContext WeightedOBESolver::synthesis(const std::vector<Example> &example_list, TimeGuard *guard) {
    std::unordered_map<std::string, ExampleList> example_pool;
    for (auto& invoke_info: invoke_map) {
        std::string name = invoke_info.first;
        std::unordered_set<std::string> feature_set;
        ExampleList res;
        for (const auto& p: invoke_info.second) {
            for (const auto& example: example_list) {
                Example invoke_example;
                for (const auto& sub: p->sub_list) {
                    invoke_example.push_back(spec->env->run(sub.get(), example));
                }
                auto feature = data::dataList2String(invoke_example);
                if (feature_set.find(feature) == feature_set.end()) {
                    res.push_back(invoke_example);
                    feature_set.insert(feature);
                }
            }
        }
        std::random_shuffle(res.begin(), res.end());
        example_pool[name] = res;
    }

    Env* env = spec->env.get();
    auto* obe_optimizer = new CachedOBEOptimizer(is_runnable, example_pool, env);
    auto* finite_example_space = new FiniteExampleSpace(spec->example_space->cons_program, example_list, env);
    auto* finite_verifier = new FiniteExampleVerifier(finite_example_space);

    EnumConfig c(finite_verifier, obe_optimizer, guard);

    auto res = _weightedEnumerate(spec->info_list, c);

    delete obe_optimizer;
    delete finite_example_space;
    delete finite_verifier;
    return res;
}