//
// Created by pro on 2022/11/18.
//

#include <istool/basic/bitset.h>
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/solver/autolifter/basic/streamed_example_space.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/invoker/invoker.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolifter;

CombinatorCase::CombinatorCase(const Bitset &_null_inputs, const PProgram &_program): null_inps(_null_inputs), program(_program) {
}

namespace {
    struct _OutputCase {
        std::vector<int> path;
        PProgram program;
        Env* env;
        _OutputCase(const std::vector<int>& _path, const PProgram& _program, Env* _env): path(_path), program(_program), env(_env) {}
        Data extract(const Data& d) const {
            auto res = d;
            for (auto pos: path) {
                auto* cv = dynamic_cast<VCompress*>(res.get());
                if (cv) {
                    try {
                        return env->run(program.get(), {cv->content});
                    } catch (SemanticsError& e) {
                        return {};
                    }
                }
                auto* tv = dynamic_cast<VTuple*>(res.get());
                assert(tv);
                res = tv->elements[pos];
            }
            return res;
        }
    };

    void _collectOutputCase(const Ty& type, std::vector<int>& path, std::vector<_OutputCase>& res, const std::vector<FRes>& f_res_list, Env* env) {
        if (type->getType() == TyType::TUPLE) {
            auto* tt = dynamic_cast<TyTuple*>(type.get());
            for (int i = 0; i < tt->fields.size(); ++i) {
                path.push_back(i);
                _collectOutputCase(tt->fields[i], path, res, f_res_list, env);
                path.pop_back();
            }
        } else if (type->getType() == TyType::COMPRESS) {
            auto* ct = dynamic_cast<TyLabeledCompress*>(type.get());
            assert(ct);
            for (int i = 0; i < f_res_list[ct->id].component_list.size(); ++i) {
                path.push_back(i);
                auto& oup_component = f_res_list[ct->id].component_list[i];
                res.emplace_back(path, oup_component.program, env);
                path.pop_back();
            }
        } else res.emplace_back(path, nullptr, env);
    }

    std::vector<_OutputCase> _collectOutputCase(int pass_id, IncreAutoLifterSolver* solver) {
        auto oup_type = solver->info->pass_infos[pass_id]->oup_type;
        std::vector<int> path; std::vector<_OutputCase> res;
        _collectOutputCase(oup_type, path, res, solver->f_res_list, solver->env.get());
        return res;
    }

    class CExampleSpace {
    public:
        FExampleSpace* base_example_space;
        PEnv env;
        std::unordered_map<std::string, int> id_map;
        std::vector<std::pair<Bitset, IOExampleList>> example_storage;
        TypeList inp_type_list;
        Ty oup_ty;
        std::vector<FRes> f_res_list;
        ProgramStorage compress_program_storage;
        ProgramList const_program_list;
        int KExampleTimeOut = 10, current_pos, KExampleEnlargeFactor = 2;

        Bitset getExampleFeature(const DataList& inp) {
            Bitset S(inp.size(), false);
            for (int i = 0; i < inp.size(); ++i) {
                if (inp[i].isNull()) S.set(i, true);
            }
            return S;
        }

        void insertExample(const IOExample& example) {
            auto S = getExampleFeature(example.first);
            auto feature = S.toString();
            auto it = id_map.find(feature);
            if (it == id_map.end()) {
                id_map[feature] = example_storage.size();
                example_storage.emplace_back(S, (IOExampleList){example});
            } else example_storage[it->second].second.push_back(example);
        }
        Data runOutput(const Ty& type, const Data& d) {
            if (type->getType() == TyType::TUPLE) {
                auto* tt = dynamic_cast<TyTuple*>(type.get());
                auto* tv = dynamic_cast<VTuple*>(d.get());
                assert(tt && tv && tt->fields.size() == tv->elements.size());
                DataList elements(tt->fields.size());
                for (int i = 0; i < tt->fields.size(); ++i) {
                    elements[i] = runOutput(tt->fields[i], tv->elements[i]);
                }
                return BuildData(Product, elements);
            }
            if (type->getType() == TyType::COMPRESS) {
                auto* ct = dynamic_cast<TyLabeledCompress*>(type.get());
                auto* cv = dynamic_cast<VCompress*>(d.get());
                assert(ct && cv); auto& res = f_res_list[ct->id];
                DataList elements(res.component_list.size());
                for (int i = 0; i < res.component_list.size(); ++i) {
                    elements[i] = env->run(res.component_list[i].program.get(), {cv->content});
                }
                return BuildData(Product, elements);
            }
            return d;
        }
        void buildExample(int example_id) {
#ifdef DEBUG
            assert(example_id < base_example_space->example_list.size());
#endif
            auto& f_example = base_example_space->example_list[example_id];
            DataList inp;
            for (int i = 0; i < compress_program_storage.size(); ++i) {
                for (auto& d: f_example.compress_storage[i]) {
                    for (auto& p: compress_program_storage[i]) {
                        try {
                            inp.push_back(env->run(p.get(), {d}));
                        } catch (SemanticsError& e) {
                            inp.emplace_back();
                        }
                    }
                }
            }
            for (auto& p: const_program_list) {
                inp.push_back(env->run(p.get(), f_example.const_list));
            }
            auto oup = runOutput(oup_ty, f_example.oup);
            insertExample({inp, oup});
        }
        CExampleSpace(int pass_id, FExampleSpace* _base_example_space, IncreAutoLifterSolver* source):
                base_example_space(_base_example_space), compress_program_storage(_base_example_space->compress_infos.size()) {
            env = base_example_space->env;
            f_res_list = source->f_res_list; oup_ty = source->info->pass_infos[pass_id]->oup_type;

            for (int i = 0; i < base_example_space->compress_infos.size(); ++i) {
                int compress_id = base_example_space->compress_infos[i].first;
                for (auto& f_res: source->f_res_list[compress_id].component_list) {
                    compress_program_storage[i].push_back(f_res.program);
                }
            }

            const_program_list = source->const_res_list[pass_id].const_list;
            int init_example_num = base_example_space->example_list.size();
            current_pos = (init_example_num + 1) / KExampleEnlargeFactor;

            // collect input types
            for (int i = 0; i < compress_program_storage.size(); ++i) {
                for (auto _: base_example_space->compress_infos[i].second) {
                    for (auto cp: compress_program_storage[i]) {
                        inp_type_list.push_back(theory::clia::getTInt());
                    }
                }
            }
            for (auto& const_inp: base_example_space->const_infos) {
                inp_type_list.push_back(const_inp.second);
            }

            for (int i = 0; i < current_pos; ++i) {
                buildExample(i);
            }
        }

        int extendExample() {
            int target_num = current_pos * KExampleEnlargeFactor;
            auto* guard = new TimeGuard(KExampleTimeOut);
            target_num = base_example_space->acquireExample(target_num, guard);
            delete guard;
            for (;current_pos < target_num; ++current_pos) {
                buildExample(current_pos);
            }
            return target_num;
        }

        bool isValid(int case_id, const PProgram& program) {
            for (auto& [inp, oup]: example_storage[case_id].second) {
                if (!(env->run(program.get(), inp) == oup)) {
                    return false;
                }
            }
            return true;
        }
    };

    PProgram _buildPossibleNullProgram(const PProgram& cond, const PProgram& res, Env* env) {
        auto null = program::buildConst({});
        return std::make_shared<Program>(env->getSemantics("ite"), (ProgramList){cond, null, res});
    }

    typedef std::pair<std::vector<int>, PProgram> ComponentProgram;

    PProgram _mergeComponentProgram(int pos, int l, int r, const std::vector<ComponentProgram>& component_list, Env* env) {
        if (r == l + 1 && pos == component_list[l].first.size()) return component_list[l].second;
        int pre = l;
        ProgramList components;
        for (int i = l + 1; i <= r; ++i) {
            if (i == r || component_list[i].first[pos] != component_list[pre].first[pos]) {
                components.push_back(_mergeComponentProgram(pos + 1, pre, i, component_list, env));
                pre = i;
            }
        }
        return std::make_shared<Program>(env->getSemantics("prod"), components);
    }

    PProgram _mergeComponentProgram(const std::vector<ComponentProgram>& component_list, Env* env) {
        for (int i = 1; i < component_list.size(); ++i) {
            assert(component_list[i - 1].first < component_list[i].first);
        }
        return _mergeComponentProgram(0, 0, component_list.size(), component_list, env);
    }

    Grammar* _eraseNullParam(Grammar* base, const Bitset& null_params) {
        base->indexSymbol();
        NTList symbols(base->symbol_list.size());
        for (int i = 0; i < symbols.size(); ++i) {
            symbols[i] = new NonTerminal(base->symbol_list[i]->name, base->symbol_list[i]->type);
        }
        for (auto* base_symbol: base->symbol_list) {
            auto* symbol = symbols[base_symbol->id];
            for (auto* rule: base_symbol->rule_list) {
                auto* cr = dynamic_cast<ConcreteRule*>(rule); assert(cr);
                auto* ps = dynamic_cast<ParamSemantics*>(cr->semantics.get());
                if (ps && null_params[ps->id]) continue;
                NTList params;
                for (auto& base_param: rule->param_list) params.push_back(symbols[base_param->id]);
                symbol->rule_list.push_back(new ConcreteRule(cr->semantics, params));
            }
        }
        return new Grammar(symbols[base->start->id], symbols);
    }

    TypeList _getInpTypes(Grammar* grammar) {
        TypeList res;
        for (auto* symbol: grammar->symbol_list) {
            for (auto* rule: symbol->rule_list) {
                auto* cr = dynamic_cast<ConcreteRule*>(rule); assert(cr);
                auto* ps = dynamic_cast<ParamSemantics*>(cr->semantics.get());
                if (ps) {
                    while (res.size() <= ps->id) res.emplace_back();
                    if (res[ps->id]) assert(res[ps->id]->equal(ps->oup_type.get()));
                    else res[ps->id] = ps->oup_type;
                }
            }
        }
        for (int i = 0; i < res.size(); ++i) {
            if (!res[i]) LOG(WARNING) << "Unused parameter " << i;
        }
        return res;
    }

    PProgram _synthesis(Grammar* base_grammar, const IOExampleList& example_list, const Bitset& null_params,
            const PEnv& env, SolverToken token) {
        auto* grammar = _eraseNullParam(base_grammar, null_params);
        const std::string default_name = "func";
        auto example_space = example::buildFiniteIOExampleSpace(example_list, default_name, env.get());
        auto info = std::make_shared<SynthInfo>(default_name, _getInpTypes(base_grammar), theory::clia::getTInt(), grammar);
        auto* spec = new Specification({info}, env, example_space);
        auto* v = new FiniteExampleVerifier(example_space.get());

        auto res = invoker::synthesis(spec, v, token, nullptr);
        delete v; delete spec; delete grammar;
        return res[default_name];
    }

    std::string _path2String(const std::vector<int>& path) {
        std::string res = "[";
        for (int i = 0; i < path.size(); ++i) {
            if (i) res += ","; res += std::to_string(path[i]);
        }
        return res + "]";
    }

    PProgram _synthesisCombinatorCase(CExampleSpace* example_space, int case_id,
            const std::vector<_OutputCase>& component_info_list, Grammar* base_main_grammar, Grammar* base_cond_grammar) {
        std::vector<ComponentProgram> res_list;
        for (auto& component_info: component_info_list) {
            bool is_possible_null = false;
            IOExampleList main_example_list, cond_example_list;

            for (auto& [inp, oup]: example_space->example_storage[case_id].second) {
                auto oup_component = component_info.extract(oup);
                if (oup_component.isNull()) {
                    is_possible_null = true;
                    cond_example_list.emplace_back(inp, BuildData(Bool, true));
                } else {
                    cond_example_list.emplace_back(inp, BuildData(Bool, false));
                    main_example_list.emplace_back(inp, oup_component);
                }
            }

            PProgram main = _synthesis(base_main_grammar, main_example_list, example_space->example_storage[case_id].first,
                    example_space->env, SolverToken::POLYGEN);
            if (is_possible_null) {
                PProgram cond = _synthesis(base_cond_grammar, cond_example_list, example_space->example_storage[case_id].first,
                        example_space->env, SolverToken::POLYGEN_CONDITION);
                main = _buildPossibleNullProgram(cond, main, example_space->env.get());
            }
            res_list.emplace_back(component_info.path, main);
        }
        return _mergeComponentProgram(res_list, example_space->env.get());
    }
}

autolifter::CombinatorRes IncreAutoLifterSolver::synthesisCombinator(int pass_id) {
    auto* example_space = new CExampleSpace(pass_id, example_space_list[pass_id], this);
    auto output_cases = _collectOutputCase(pass_id, this);
    LOG(INFO) << "Output cases " << output_cases.size();
    for (auto& component: output_cases) {
        LOG(INFO) << "  " << _path2String(component.path) << " " << component.program;
        if (component.program) LOG(INFO) << "  " << component.program->toString();
    }

    LOG(INFO) << "program list " << type::typeList2String(example_space->inp_type_list);
    Grammar* base_grammar = buildCombinatorGrammar(example_space->inp_type_list);
    Grammar* cond_grammar = buildCombinatorCondGrammar(example_space->inp_type_list);

    LOG(INFO) << "Base grammar"; base_grammar->print();
    LOG(INFO) << "Cond grammar"; cond_grammar->print();


    ProgramList res_list;

    while (true) {
        bool is_all_valid = true;

        for (int case_id = 0; case_id < example_space->example_storage.size(); ++case_id) {
            if (res_list.size() <= case_id) res_list.emplace_back();
            else if (example_space->isValid(case_id, res_list[case_id])) continue;
            is_all_valid = false;
            res_list[case_id] = _synthesisCombinatorCase(example_space, case_id, output_cases, base_grammar, cond_grammar);
        }

        if (is_all_valid) break;
        example_space->extendExample();
    }

    LOG(INFO) << "Pass #" << pass_id;
    for (int i = 0; i < res_list.size(); ++i) {
        LOG(INFO) << " Case " << example_space->example_storage[i].first.toString() << ": " << res_list[i]->toString();
    }

    CombinatorRes comb_res;
    for (int i = 0; i < res_list.size(); ++i) {
        comb_res.emplace_back(example_space->example_storage[i].first, res_list[i]);
    }
    return comb_res;
}

void IncreAutoLifterSolver::solveCombinators() {
    for (int pass_id = 0; pass_id < info->pass_infos.size(); ++pass_id) {
        for (auto& unit: unit_storage[pass_id]) {
            auto res = synthesisCombinator(pass_id);
        }
    }
    exit(0);
}