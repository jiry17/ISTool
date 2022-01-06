//
// Created by pro on 2022/1/7.
//

#include "istool/solver/polygen/polygen_unifier.h"
#include "glog/logging.h"
#include <algorithm>

namespace {
    const bool KDefaultIsUseTerm = true;
}

PolyGenUnifier::PolyGenUnifier(Specification *spec, const PSynthInfo& info, const SolverBuilder &_builder):
    Unifier(spec, info), builder(_builder) {
    auto* val = spec->env->getConstRef(solver::polygen::KIsUseTermName);
    if (val->isNull()) KIsUseTerm = KDefaultIsUseTerm; else KIsUseTerm = val->isTrue();
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "PolyGenTermSolver require the number of target programs to be 1";
    }
    io_space = dynamic_cast<IOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "PolyGenTermSolver supports only IOExamples";
    }
}

namespace {
    PSynthInfo _insertTermsToInfo(const PSynthInfo& info, int term_num, const PType& term_type) {
        Grammar* g = grammar::copyGrammar(info->grammar);
        TypeList inp_type = info->inp_type_list;
        for (auto* symbol: g->symbol_list) {
            if (!term_type->equal(symbol->type.get())) continue;
            for (int i = 0; i < term_num; ++i) {
                int id = i + int(inp_type.size());
                symbol->rule_list.push_back(new Rule(semantics::buildParamSemantics(id, term_type), {}));
            }
        }
        for (int i = 0; i < term_num; ++i) inp_type.push_back(term_type);
        return std::make_shared<SynthInfo>(info->name, inp_type, type::getTBool(), g);
    }

    void _insertTermsToExample(IOExampleList& example_list, const ProgramList& term_list) {
        for (int i = 0; i < example_list.size(); ++i) {
            // todo: handle SemanticsError
            DataList term_results;
            for (const auto& term: term_list) {
                term_results.push_back(program::run(term.get(), example_list[i].first));
            }
            for (auto& d: term_results) {
                example_list[i].first.push_back(d);
            }
        }
    }

    ProgramList _reorderTermList(const ProgramList& term_list, const IOExampleList& example_list) {
        std::vector<std::pair<int, PProgram>> info_list;
        for (const auto& term: term_list) {
            int num = 0;
            for (const auto& example: example_list) {
                if (example::satisfyIOExample(term.get(), example)) ++num;
            }
            info_list.emplace_back(num, term);
        }
        std::sort(info_list.begin(), info_list.end());
        ProgramList result;
        for (auto& info: info_list) result.push_back(info.second);
        return result;
    }

    PProgram _buildIte(const ProgramList& term_list, const ProgramList& condition_list, Env* env) {
        int n = condition_list.size();
        auto res = term_list[n];
        auto semantics = env->getSemantics("ite");
        for (int i = n - 1; i >= 0; --i) {
            ProgramList sub_list = {condition_list[i], term_list[i], res};
            res = std::make_shared<Program>(semantics, sub_list);
        }
        return res;
    }
}

PProgram PolyGenUnifier::synthesizeCondition(const PSynthInfo& info, const IOExampleList &positive_list,
        const IOExampleList &negative_list, TimeGuard* guard) {
    IOExampleList example_list;
    for (auto& example: positive_list) example_list.emplace_back(example.first, BuildData(Bool, true));
    for (auto& example: negative_list) example_list.emplace_back(example.first, BuildData(Bool, false));
    auto example_space = example::buildFiniteIOExampleSpace(example_list, info->name, spec->env.get());
    auto* pred_spec = new Specification({info}, spec->env, example_space);
    auto* solver = builder(pred_spec);
    auto res = solver->synthesis(guard);
    delete pred_spec; delete solver;
    return res.begin()->second;
}

PProgram PolyGenUnifier::unify(const ProgramList &raw_term_list, const ExampleList &example_list, TimeGuard *guard) {
    PSynthInfo pred_info = unify_info;
    IOExampleList io_example_list;
    for (const auto& example: example_list) {
        io_example_list.push_back(io_space->getIOExample(example));
    }
    ProgramList term_list = _reorderTermList(raw_term_list, io_example_list);
    if (KIsUseTerm) {
        pred_info = _insertTermsToInfo(unify_info, term_list.size(), spec->info_list[0]->oup_type);
        _insertTermsToExample(io_example_list, term_list);
    }
    PExampleSpace example_space = example::buildFiniteIOExampleSpace(io_example_list, spec->info_list[0]->name, spec->env.get());

    ProgramList condition_list;
    for (int term_id = 0; term_id + 1 < term_list.size(); ++term_id) {
        TimeCheck(guard);
        auto term = term_list[term_id];
        IOExampleList positive_list, negative_list, free_list;
        for (const auto& example: io_example_list) {
            if (!example::satisfyIOExample(term.get(), example)) {
                negative_list.push_back(example);
            } else {
                bool is_free = false;
                for (int i = term_id + 1; i < term_list.size(); ++i) {
                    if (example::satisfyIOExample(term_list[i].get(), example)) {
                        is_free = true; break;
                    }
                }
                if (is_free) free_list.push_back(example); else positive_list.push_back(example);
            }
        }

        PProgram res = synthesizeCondition(pred_info, positive_list, negative_list, guard );
        condition_list.push_back(res);

        io_example_list = negative_list;
        for (const auto& example: free_list) {
            if (!program::run(res.get(), example.first).isTrue()) {
                io_example_list.push_back(example);
            }
        }
    }

    if (KIsUseTerm) {
        int n = unify_info->inp_type_list.size();
        ProgramList param_list(n + int(term_list.size()));
        for (int i = 0; i < term_list.size(); ++i) {
            param_list[i + n] = term_list[i];
        }
        for (int i = 0; i < condition_list.size(); ++i) {
            condition_list[i] = program::rewriteParam(condition_list[i], param_list);
        }
    }

    return _buildIte(term_list, condition_list, spec->env.get());
}

const std::string solver::polygen::KIsUseTermName = "PolyGen@IsUseTerm";