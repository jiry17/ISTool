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
#include <iostream>

using namespace incre;
using namespace incre::autolifter;

namespace {
    struct _OutputCase {
        std::vector<int> path;
        TypedProgram program;
        Env* env;
        _OutputCase(const std::vector<int>& _path, const TypedProgram& _program, Env* _env): path(_path), program(_program), env(_env) {}
        Data extract(const Data& d) const {
            auto res = d;
            for (auto pos: path) {
                auto* cv = dynamic_cast<VCompress*>(res.get());
                if (cv) {
                    try {
                        return env->run(program.second.get(), {cv->content});
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
            if (f_res_list[ct->id].component_list.size() == 1) {
                res.emplace_back(path, f_res_list[ct->id].component_list[0].program,env);
            } else {
                for (int i = 0; i < f_res_list[ct->id].component_list.size(); ++i) {
                    path.push_back(i);
                    auto &oup_component = f_res_list[ct->id].component_list[i];
                    res.emplace_back(path, oup_component.program, env);
                    path.pop_back();
                }
            }
        } else res.emplace_back(path, std::pair<PType, PProgram>(incre::typeFromIncre(type), nullptr), env);
    }

    std::vector<_OutputCase> _collectOutputCase(int align_id, IncreAutoLifterSolver* solver) {
        auto oup_type = solver->info->align_infos[align_id]->oup_type;
        std::vector<int> path; std::vector<_OutputCase> res;
        _collectOutputCase(oup_type, path, res, solver->f_res_list, solver->env.get());
        return res;
    }

    class CExampleSpace {
    public:
        FExampleSpace* base_example_space;
        PEnv env;
        int align_id;

        IOExampleList example_list;
        TypeList inp_type_list;
        Ty oup_ty;
        std::vector<FRes> f_res_list;
        TypedProgramList compress_program_list;
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
                assert(ct && cv);
                return f_res_list[ct->id].run(cv->content, env.get());
            }
            return d;
        }
        void buildExample(int example_id) {
#ifdef DEBUG
            assert(example_id < base_example_space->example_list.size());
#endif
            auto& f_example = base_example_space->example_list[example_id];
            auto& original_input = f_example.first;

            DataList inp;

            for (auto& [compress_type, compress_program]: compress_program_list) {
                auto* ltc = dynamic_cast<TLabeledCompress*>(compress_type.get());
                auto compress_res = env->run(compress_program.get(), original_input);
                if (!ltc) {
                    inp.push_back(compress_res);
                } else {
                    auto* lv = dynamic_cast<VLabeledCompress*>(compress_res.get());
                    assert(lv && ltc->id == lv->id);
                    for (auto& aux_program: f_res_list[lv->id].component_list) {
                        auto aux_value = env->run(aux_program.program.second.get(), {lv->content});
                        inp.push_back(aux_value);
                    }
                }
            }

            auto oup = runOutput(oup_ty, f_example.second);
            insertExample({inp, oup});
        }
        CExampleSpace(int _align_id, FExampleSpace* _base_example_space, IncreAutoLifterSolver* source):
                align_id(_align_id), base_example_space(_base_example_space) {
            env = source->env; f_res_list = source->f_res_list;
            oup_ty = source->info->align_infos[align_id]->oup_type;
            compress_program_list = source->compress_res_list[align_id].compress_list;

            int init_example_num = base_example_space->example_list.size();
            current_pos = (init_example_num + 1) / KExampleEnlargeFactor;

            // collect input types
            for (auto& [compress_type, compress_program]: compress_program_list) {
                auto* ltc = dynamic_cast<TLabeledCompress*>(compress_type.get());
                if (!ltc) {
                    inp_type_list.push_back(compress_type);
                } else {
                    for (auto& aux_program: f_res_list[ltc->id].component_list) {
                        inp_type_list.push_back(aux_program.program.first);
                    }
                }
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

    PProgram _mergeComponentProgram(TyData* type, int& pos, const ProgramList& program_list, const std::vector<FRes>& f_res_list, Env* env) {
        if (type->getType() == TyType::COMPRESS) {
            auto* ct = dynamic_cast<TyLabeledCompress*>(type); assert(ct);
            int size = f_res_list[ct->id].component_list.size();
            if (size == 0) return program::buildConst(Data(std::make_shared<VUnit>()));
            if (size == 1) return program_list[pos++];
            ProgramList sub_list;
            for (int i = 0; i < size; ++i) sub_list.push_back(program_list[pos++]);
            return std::make_shared<Program>(env->getSemantics("prod"), sub_list);
        }
        if (type->getType() == TyType::TUPLE) {
            auto* tt = dynamic_cast<TyTuple*>(type);
            ProgramList sub_list;
            for (auto& sub: tt->fields) {
                sub_list.push_back(_mergeComponentProgram(sub.get(), pos, program_list, f_res_list, env));
            }
            return std::make_shared<Program>(env->getSemantics("prod"), sub_list);
        }
        return program_list[pos++];
    }

    PProgram _mergeComponentProgram(TyData* type, const ProgramList& program_list, const std::vector<FRes>& f_res_list, Env* env) {
        int pos = 0;
        auto res = _mergeComponentProgram(type, pos, program_list, f_res_list, env);
        assert(pos == program_list.size());
        return res;
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
        if (example_list.empty()) {
            return grammar::getMinimalProgram(grammar);
        }
        auto example_space = example::buildFiniteIOExampleSpace(example_list, default_name, env.get());
        auto info = std::make_shared<SynthInfo>(default_name, _getInpTypes(grammar), grammar->start->type, grammar);
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

    SolverToken _getSolverToken(Type* oup_type) {
        if (dynamic_cast<TBool*>(oup_type)) return SolverToken::POLYGEN_CONDITION;
        if (dynamic_cast<TInt*>(oup_type)) return SolverToken::POLYGEN;
        LOG(FATAL) << "Unsupported type " << oup_type->getName();
    }

    PProgram _synthesisCombinator(CExampleSpace* example_space, const std::vector<_OutputCase>& component_info_list, IncreAutoLifterSolver* solver) {
        ProgramList res_list;
        for (auto& component_info: component_info_list) {
            IOExampleList component_example_list;
            for (auto& [inp, oup]: example_space->example_list) {
                auto oup_component = component_info.extract(oup);
                component_example_list.emplace_back(inp, oup_component);
            }

            // synthesis
            auto oup_type = component_info.program.first;
            auto* grammar = solver->buildCombinatorGrammar(example_space->inp_type_list, oup_type, example_space->align_id);
            SolverToken token = _getSolverToken(oup_type.get());
            PProgram main = _synthesis(grammar, component_example_list, example_space->env, token);
            res_list.push_back(main);
        }
        return _mergeComponentProgram(example_space->oup_ty.get(), res_list, example_space->f_res_list, example_space->env.get());
    }
}

namespace {
    Term _buildProgram(Program* program, const std::unordered_map<std::string, SynthesisComponent*>& component, const TermList& param_list) {
        auto *ps = dynamic_cast<ParamSemantics *>(program->semantics.get());
        if (ps) return param_list[ps->id];
        TermList sub_list;
        for (const auto& sub: program->sub_list) sub_list.push_back(_buildProgram(sub.get(), component, param_list));
        auto* as = dynamic_cast<AccessSemantics*>(program->semantics.get());
        if (as) {
            assert(sub_list.size() == 1);
            return std::make_shared<TmProj>(sub_list[0], as->id + 1);
        }
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

    Term _buildProgram(const PProgram& program, const std::vector<SynthesisComponent*>& component_list,
                       const TermList& param_list) {
        std::unordered_map<std::string, SynthesisComponent*> component_map;
        for (auto* component: component_list) {
            component_map[component->semantics->getName()] = component;
        }
        return _buildProgram(program.get(), component_map, param_list);
    }

    bool _isSymbolTerm(TermData* term) {
        return term->getType() == TermType::VALUE || term->getType() == TermType::VAR;
    }
}

Term IncreAutoLifterSolver::synthesisCombinator(int align_id) {
    auto* example_space = new CExampleSpace(align_id, example_space_list[align_id], this);
    auto output_cases = _collectOutputCase(align_id, this);
    LOG(INFO) << "Output cases " << output_cases.size();
    for (auto& component: output_cases) {
        std::cout << "  " << _path2String(component.path) << " ";
        if (!component.program.second) std::cout << "null" << std::endl;
        else std::cout << component.program.second->toString() << std::endl;
    }
    PProgram res = nullptr;

    while (!res || !example_space->isValid(res)) {
        res = _synthesisCombinator(example_space, output_cases, this);
        LOG(INFO) << "candidate program " << res->toString();
        example_space->extendExample();
    }
    LOG(INFO) << "finished";

    // Build Param List
    TermList compress_param_list;
    for (auto& [var_name, var_type]: info->align_infos[align_id]->inp_types) {
        compress_param_list.push_back(std::make_shared<TmVar>(var_name));
    }

    std::vector<std::pair<std::string, Term>> binding_list;
    TermList combine_param_list;
    int var_id = 0;

    for (auto& [compress_type, compress_program]: compress_res_list[align_id].compress_list) {
        LOG(INFO) << "build compress " << compress_program->toString();
        for (auto& param: compress_param_list) LOG(INFO) << "  param: " << param->toString();
        auto compress_term = _buildProgram(compress_program, info->component_list, compress_param_list);
        if (!_isSymbolTerm(compress_term.get())) {
            std::string compress_name = "c" + std::to_string(var_id++);
            binding_list.emplace_back(compress_name, compress_term);
            compress_term = std::make_shared<TmVar>(compress_name);
        }

        auto* lt = dynamic_cast<TLabeledCompress*>(compress_type.get());
        if (!lt) {
            combine_param_list.push_back(compress_term);
        } else {
            int compress_id = lt->id;
            int component_num = f_res_list[compress_id].component_list.size();
            if (!component_num) continue;
            if (component_num == 1) {
                combine_param_list.push_back(compress_term);
            } else {
                for (int i = 1; i <= component_num; ++i) {
                    combine_param_list.push_back(std::make_shared<TmProj>(compress_term, i));
                }
            }
        }
    }

    auto term = _buildProgram(res, info->component_list, combine_param_list);
    std::reverse(binding_list.begin(), binding_list.end());
    for (auto& [name, binding]: binding_list) {
        term = std::make_shared<TmLet>(name, binding, term);
    }

    return term;
}

void IncreAutoLifterSolver::solveCombinators() {
    for (int pass_id = 0; pass_id < info->align_infos.size(); ++pass_id) {
        comb_list.push_back(synthesisCombinator(pass_id));
    }
}