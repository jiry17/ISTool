//
// Created by pro on 2022/1/4.
//

#include "istool/solver/polygen/lia_solver.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/solver/enum/enum.h"
#include "glog/logging.h"
#include <unordered_set>
#include <algorithm>

const std::string solver::lia::KConstIntMaxName = "LIA@ConstIntMax";
const std::string solver::lia::KTermIntMaxName = "LIA@TermIntMax";

namespace {
    const int KDefaultValue = 2;

    int _getDefaultConstMax(Grammar* g) {
        int c_max = KDefaultValue;
        for (auto* symbol: g->symbol_list) {
            for (auto* rule: symbol->rule_list) {
                auto* cs = dynamic_cast<ConstSemantics*>(rule->semantics.get());
                if (!cs) continue;
                auto* iv = dynamic_cast<IntValue*>(cs->w.get());
                if (iv) c_max = std::max(c_max, std::abs(iv->w));
            }
        }
        return c_max;
    }
}

LIAResult::LIAResult(): status(false), c_val(0) {}
LIAResult::LIAResult(const std::vector<int> &_param_list, int _c_val): status(true), c_val(_c_val), param_list(_param_list) {
}

namespace {
    PSynthInfo _buildLIAInfo(const PSynthInfo &info) {
        auto *grammar = grammar::copyGrammar(info->grammar);
        grammar->indexSymbol();
        std::string name = grammar::getFreeName(grammar);
        auto *start = new NonTerminal(name, grammar->start->type);
        for (auto *rule: grammar->start->rule_list) {
            if (rule->semantics->name == "+" || rule->semantics->name == "-") continue;
            NTList sub_list = rule->param_list;
            start->rule_list.push_back(new Rule(rule->semantics, std::move(sub_list)));
        }
        int n = grammar->symbol_list.size();
        grammar->symbol_list.push_back(start);
        std::swap(grammar->symbol_list[0], grammar->symbol_list[n]);
        grammar->start = start;
        return std::make_shared<SynthInfo>(info->name, info->inp_type_list, info->oup_type, grammar);
    }
}

LIASolver::LIASolver(Specification *_spec, const ProgramList &_program_list, int _KTermIntMax, int _KConstIntMax, double _KRelaxTimeLimit):
    PBESolver(_spec), ext(ext::z3::getExtension(_spec->env.get())), KTermIntMax(_KTermIntMax), program_list(_program_list),
    KConstIntMax(_KConstIntMax), KRelaxTimeLimit(_KRelaxTimeLimit) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "LIA Solver can only synthesize a single program";
    }
    io_example_space = dynamic_cast<IOExampleSpace*>(spec->example_space.get());
    if (!io_example_space) {
        LOG(FATAL) << "LIA solver supports only IOExampleSpace";
    }
    auto term_info = spec->info_list[0];
    for (const auto& type: term_info->inp_type_list) {
        if (!dynamic_cast<TInt*>(type.get())) {
            LOG(FATAL) << "LIA solver supports only integers";
        }
    }
    if (!dynamic_cast<TInt*>(term_info->oup_type.get())) {
        LOG(FATAL) << "LIA solver supports only integers";
    }
    info = _buildLIAInfo(term_info);
}

namespace {
    PProgram _times(int c, const PProgram& y, Env* env) {
        if (c == 1) return y;
        ProgramList sub_list = {program::buildConst(BuildData(Int, c)), y};
        return std::make_shared<Program>(env->getSemantics("*"), sub_list);
    }

    PProgram _add(const PProgram& x, int c, const PProgram& y, Env* env) {
        if (c == 0) return x;
        if (!x) return _times(c, y, env);
        ProgramList sub_list = {x, _times(std::abs(c), y, env)};
        std::string name = c > 0 ? "+" : "-";
        return std::make_shared<Program>(env->getSemantics(name), sub_list);
    }

    PProgram _buildProgram(const LIAResult& solve_res, const ProgramList& program_list, Env* env) {
        PProgram res;
        auto plus = env->getSemantics("+"), times = env->getSemantics("*");
        if (solve_res.c_val) res = program::buildConst(BuildData(Int, solve_res.c_val));
        for (int i = 0; i < solve_res.param_list.size(); ++i) {
            res = _add(res, solve_res.param_list[i], program_list[i], env);
        }
        if (!res) res = program::buildConst(BuildData(Int, 0));
        return res;
    }
}

FunctionContext LIASolver::synthesis(const std::vector<Example> &example_list, TimeGuard *guard) {
    if (example_list.empty()) {
        return semantics::buildSingleContext(io_example_space->func_name, program::buildConst(BuildData(Int, 0)));
    }
    IOExampleList io_example_list;
    for (const auto& example: example_list) {
        io_example_list.push_back(io_example_space->getIOExample(example));
    }
    std::unordered_set<std::string> cache;
    ProgramList considered_program_list;
    IOExampleList wrapped_example_list;
    for (auto& io_example: io_example_list) {
        wrapped_example_list.emplace_back(DataList(), io_example.second);
    }
    for (const auto& program: program_list) {
        DataList output_list;
        for (const auto& io_example: io_example_list) {
            output_list.push_back(program::run(program.get(), io_example.first));
        }
        auto feature = data::dataList2String(output_list);
        if (cache.find(feature) == cache.end()) {
            cache.insert(feature);
            for (int i = 0; i < output_list.size(); ++i) {
                wrapped_example_list[i].first.push_back(output_list[i]);
            }
            considered_program_list.push_back(program);
        }
    }

    auto solve_res = solver::lia::solveLIA(wrapped_example_list, ext, KConstIntMax, KTermIntMax, guard);
    if (!solve_res.status) return {};

    PProgram res = _buildProgram(solve_res, considered_program_list, spec->env.get());
    return semantics::buildSingleContext(io_example_space->func_name, res);
}

LIAResult solver::lia::solveLIA(const std::vector<IOExample> &example_list, Z3Extension *ext, int c_max, int t_max, TimeGuard *guard) {
    int n = example_list[0].first.size();
    z3::optimize o(ext->ctx);
    std::vector<z3::expr> param_list;
    for (int i = 0; i < n; ++i) {
        std::string var_name = "x" + std::to_string(i);
        auto var = ext->ctx.int_const(var_name.c_str());
        o.add(var >= -t_max && var <= t_max);
        param_list.push_back(var);
    }
    auto const_var = ext->ctx.int_const("c");
    o.add(const_var >= -c_max && const_var <= c_max);

    for (auto& example: example_list) {
        z3::expr res = const_var;
        for (int i = 0; i < n; ++i) {
            res = res + theory::clia::getIntValue(example.first[i]) * param_list[i];
        }
        o.add(res == theory::clia::getIntValue(example.second));
    }

    z3::expr weight = z3::ite(const_var == 0, ext->ctx.int_val(0), ext->ctx.int_val(1));
    for (const auto& var: param_list) weight = weight + z3::abs(var);
    auto h = o.minimize(weight);

    TimeCheck(guard);
    if (guard) {
        double remain_time = std::max(guard->getRemainTime() * 1e3, 1.0);
        z3::params p(ext->ctx);
        p.set(":timeout", int(remain_time) + 1u);
        o.set(p);
    }

    auto status = o.check();
    if (status == z3::unsat) return {};
    o.lower(h); auto model = o.get_model();

    auto type = theory::clia::getTInt();
    auto const_val = ext->getValueFromModel(model, const_var, type.get());
    auto const_w = theory::clia::getIntValue(const_val);

    std::vector<int> param_val_list;
    for (int i = 0; i < n; ++i) {
        auto val = ext->getValueFromModel(model, param_list[i], type.get());
        param_val_list.push_back(theory::clia::getIntValue(val));
    }

    return {param_val_list, const_w};
}

namespace {
    Program* _getNextOperand(Program* program) {
        if (program->semantics->name == "*") return program->sub_list[0].get();
        return program;
    }

    bool _isDuplicated(Program* program) {
        // (p1 + p2) * p3 is equivalent to p1 * p3 and p2 * p3;
        if (program->semantics->name == "+") return true;
        if (program->semantics->name == "-") return true;
        // c * p1 is equivalent to p1.
        if (dynamic_cast<ConstSemantics*>(program->semantics.get())) return true;
        if (program->semantics->name == "*") {
            // (p1 * p2) * p3 is equivalent to p1 * (p2 * p3)
            if (program->sub_list[0]->semantics->name == "*") return true;
            // p1 * (p2 * p3) /\ p1 > p2 is equivalent to p2 * (p1 * p3)
            if (program->sub_list[0]->toString() > _getNextOperand(program->sub_list[1].get())->toString()) return true;
            for (const auto& sub: program->sub_list) {
                if (_isDuplicated(sub.get())) return true;
            }
            return false;
        }
        return false;
    }

    class LIARelaxVerifier: public Verifier {
    public:
        virtual bool verify(const FunctionContext& info, Example* counter_example) {
            assert(!counter_example && info.size() == 1);
            auto program = info.begin()->second;
            if (_isDuplicated(program.get())) {
                return false;
            }
            return true;
        }
        virtual ~LIARelaxVerifier() = default;
    };

    ProgramList _getConsideredTerms(const PSynthInfo& info, int num, double time_out) {
        auto* verifier = new LIARelaxVerifier();
        auto* optimizer = new TrivialOptimizer();
        auto* tmp_guard = new TimeGuard(time_out);
        EnumConfig c(verifier, optimizer, tmp_guard);
        c.res_num_limit = num;
        auto res_list = solver::enumerate({info}, c);
        delete verifier; delete optimizer; delete tmp_guard;

        ProgramList next_program_list;
        for (auto& res: res_list) {
            next_program_list.push_back(res.begin()->second);
        }
        return next_program_list;
    }
}

void* LIASolver::relax(TimeGuard* guard) {
    int next_num = std::max(1, int(program_list.size()) * 2);
    double time_out = KRelaxTimeLimit;
    if (guard) time_out = std::min(time_out, guard->getRemainTime());
    double next_time_limit = KRelaxTimeLimit;

    auto next_program_list = _getConsideredTerms(info, next_num, time_out);
    if (next_program_list.size() < next_num) next_time_limit *= 2;

    return new LIASolver(spec, next_program_list, KTermIntMax, KConstIntMax * 2, next_time_limit);
}

LIASolver * solver::lia::liaSolverBuilder(Specification *spec) {
    auto* env = spec->env.get();

    auto* c_max_data = env->getConstRef(solver::lia::KConstIntMaxName);
    if (c_max_data->isNull()) {
        env->setConst(solver::lia::KConstIntMaxName, BuildData(Int, _getDefaultConstMax(spec->info_list[0]->grammar)));
    }
    int KConstIntMax = theory::clia::getIntValue(*c_max_data);
    auto* t_max_data = spec->env->getConstRef(solver::lia::KTermIntMaxName);
    if (t_max_data->isNull()) {
        spec->env->setConst(solver::lia::KTermIntMaxName, BuildData(Int, KDefaultValue));
    }
    int KTermIntMax = theory::clia::getIntValue(*t_max_data);
    double KRelaxTimeLimit = 0.1;
    auto info = spec->info_list[0];
    ProgramList program_list = _getConsideredTerms(info, info->inp_type_list.size(), KRelaxTimeLimit);
    return new LIASolver(spec, program_list, KTermIntMax, KConstIntMax, KRelaxTimeLimit);
}