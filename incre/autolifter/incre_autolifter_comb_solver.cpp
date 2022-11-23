//
// Created by pro on 2022/11/18.
//

#include <istool/basic/bitset.h>
#include <istool/ext/deepcoder/deepcoder_semantics.h>
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/solver/autolifter/basic/streamed_example_space.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/invoker/invoker.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolifter;

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

        IOExampleList example_list;
        TypeList inp_type_list;
        Ty oup_ty;
        std::vector<FRes> f_res_list;
        ProgramStorage compress_program_storage;
        ProgramList const_program_list;
        int KExampleTimeOut = 10, current_pos, KExampleEnlargeFactor = 2;

        void insertExample(const IOExample& example) {
            example_list.push_back(example);
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
                        inp.push_back(env->run(p.get(), {d}));
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
            for (auto& const_prog: const_program_list) {
                inp_type_list.push_back(theory::clia::getTInt());
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

        bool isValid(const PProgram& program) {
            for (auto& [inp, oup]: example_list) {
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
                if (ps) {
                    LOG(INFO) << ps->id << " " << null_params.size();
                }
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

    PProgram _synthesis(Grammar* grammar, const IOExampleList& example_list, const PEnv& env, SolverToken token) {
        const std::string default_name = "func";
        auto example_space = example::buildFiniteIOExampleSpace(example_list, default_name, env.get());
        auto info = std::make_shared<SynthInfo>(default_name, _getInpTypes(grammar), theory::clia::getTInt(), grammar);
        auto* spec = new Specification({info}, env, example_space);
        auto* v = new FiniteExampleVerifier(example_space.get());

        auto res = invoker::synthesis(spec, v, token, nullptr);
        delete v; delete spec;
        return res[default_name];
    }

    std::string _path2String(const std::vector<int>& path) {
        std::string res = "[";
        for (int i = 0; i < path.size(); ++i) {
            if (i) res += ","; res += std::to_string(path[i]);
        }
        return res + "]";
    }

    PProgram _synthesisCombinator(CExampleSpace* example_space, const std::vector<_OutputCase>& component_info_list, Grammar* grammar) {
        std::vector<ComponentProgram> res_list;
        for (auto& component_info: component_info_list) {
            IOExampleList component_example_list;
            for (auto& [inp, oup]: example_space->example_list) {
                auto oup_component = component_info.extract(oup);
                component_example_list.emplace_back(inp, oup_component);
            }
            PProgram main = _synthesis(grammar, component_example_list, example_space->env, SolverToken::POLYGEN);
            res_list.emplace_back(component_info.path, main);
        }
        return _mergeComponentProgram(res_list, example_space->env.get());
    }
}

namespace {
    Term _buildProgram(Program* program, const std::unordered_map<std::string, SynthesisComponent*>& component, const TermList& param_list) {
        auto *ps = dynamic_cast<ParamSemantics *>(program->semantics.get());
        if (ps) return param_list[ps->id];
        TermList sub_list;
        for (const auto& sub: program->sub_list) sub_list.push_back(_buildProgram(sub.get(), component, param_list));
        auto name = program->semantics->getName();
        auto it = component.find(program->semantics->getName());
        if (it != component.end()) {
            return it->second->buildTerm(sub_list);
        }
        auto* cs = dynamic_cast<ConstSemantics*>(program->semantics.get());
        if (cs) return std::make_shared<TmValue>(cs->w);
        auto* pros = dynamic_cast<ProductSemantics*>(program->semantics.get());
        if (pros) return std::make_shared<TmTuple>(sub_list);
        LOG(FATAL) << "Unknown operator " << program->semantics->getName();
    }

    Term _buildProgram(const PProgram& program, const std::vector<SynthesisComponent*>& component_list, const TermList& param_list) {
        std::unordered_map<std::string, SynthesisComponent*> component_map;
        for (auto* component: component_list) {
            component_map[component->semantics->getName()] = component;
        }
        return _buildProgram(program.get(), component_map, param_list);
    }
}

Term IncreAutoLifterSolver::synthesisCombinator(int pass_id) {
    auto* example_space = new CExampleSpace(pass_id, example_space_list[pass_id], this);
    auto output_cases = _collectOutputCase(pass_id, this);
    LOG(INFO) << "Output cases " << output_cases.size();
    for (auto& component: output_cases) {
        std::cout << "  " << _path2String(component.path) << " ";
        if (!component.program) std::cout << "null" << std::endl;
        else std::cout << component.program->toString() << std::endl;
    }
    Grammar* grammar = buildCombinatorGrammar(example_space->inp_type_list);
    PProgram res = nullptr;

    while (!res || !example_space->isValid(res)) {
        res = _synthesisCombinator(example_space, output_cases, grammar);
        example_space->extendExample();
    }

    // Build Param List
    TermList param_list;
    std::vector<std::pair<std::string, Term>> binding_list;
    {
        auto* init_pass = info->pass_infos[pass_id]->term;
        for (int i = 0; i < init_pass->names.size(); ++i) {
            binding_list.emplace_back(init_pass->names[i], init_pass->defs[i]);
        }

        auto* base_example_space = example_space_list[pass_id];
        for (auto& [id, name_list]: base_example_space->compress_infos) {
            for (auto& name: name_list) {
                if (f_res_list[id].component_list.size() == 1) {
                    param_list.push_back(std::make_shared<TmVar>(name));
                } else {
                    auto base = std::make_shared<TmVar>(name);
                    for (int i = 0; i < f_res_list[id].component_list.size(); ++i) {
                        param_list.push_back(std::make_shared<TmProj>(base, i + 1));
                    }
                }
            }
        }
        int const_id = 0;
        TermList const_param_list;
        for (auto& [const_name, _]: base_example_space->const_infos) {
            const_param_list.push_back(std::make_shared<TmVar>(const_name));
        }
        for (auto& const_program: const_res_list[pass_id].const_list) {
            std::string const_name = "c" + std::to_string(const_id++);
            binding_list.emplace_back(const_name, _buildProgram(const_program, info->component_list, const_param_list));
            param_list.push_back(std::make_shared<TmVar>(const_name));
        }
    }

    auto term = _buildProgram(res, info->component_list, param_list);
    std::reverse(binding_list.begin(), binding_list.end());
    for (auto& [name, binding]: binding_list) {
        term = std::make_shared<TmLet>(name, binding, term);
    }

    return term;
}

void IncreAutoLifterSolver::solveCombinators() {
    for (int pass_id = 0; pass_id < info->pass_infos.size(); ++pass_id) {
        comb_list.push_back(synthesisCombinator(pass_id));
    }
}