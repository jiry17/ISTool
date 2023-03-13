//
// Created by pro on 2022/1/8.
//

#include "istool/solver/polygen/polygen_cegis.h"
#include "istool/basic/semantics.h"
#include "glog/logging.h"

CEGISPolyGen::~CEGISPolyGen() {
    delete term_solver;
    delete cond_solver;
}

CEGISPolyGen::CEGISPolyGen(Specification *spec, const PSynthInfo &term_info, const PSynthInfo &unify_info,
        const PBESolverBuilder &domain_builder, const PBESolverBuilder &dnf_builder, Verifier *_v):
        VerifiedSolver(spec, _v), term_solver(new PolyGenTermSolver(spec, term_info, domain_builder)),
        cond_solver(new PolyGenConditionSolver(spec, unify_info, dnf_builder)) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "PolyGen can only synthesize a single program";
    }
    io_space = dynamic_cast<IOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "PolyGen supports only IOExamples";
    }
}

namespace {
    PProgram _mergeIte(const ProgramList& term_list, const ProgramList& cond_list, Env* env) {
        int n = cond_list.size();
        auto res = term_list[n];
        auto ite_semantics = env->getSemantics("ite");
        for (int i = n - 1; i >= 0; --i) {
            ProgramList sub_list = {cond_list[i], term_list[i], res};
            res = std::make_shared<Program>(ite_semantics, sub_list);
        }
        return res;
    }

    PProgram _handleSemanticsErrorInMid(const PProgram& c, const IOExampleList& example_list, Env* env) {
        bool is_exist_error = false;
        for (auto& example: example_list) {
            try {
                env->run(c.get(), example.first);
            } catch (SemanticsError& e) {
                is_exist_error = true;
                break;
            }
        }
        if (!is_exist_error) return c;
        return std::make_shared<Program>(std::make_shared<AllowFailSemantics>(type::getTBool(), BuildData(Bool, true)), (ProgramList){c});
    }
}

#include "istool/sygus/theory/basic/clia/clia.h"

FunctionContext CEGISPolyGen::synthesis(TimeGuard *guard) {
    // LOG(INFO) << "start";
    auto info = spec->info_list[0];
    auto start = grammar::getMinimalProgram(info->grammar);
    ExampleList example_list;
    IOExampleList io_example_list;
    Example counter_example;
    auto result = semantics::buildSingleContext(info->name, start);
    LOG(INFO) << "synthesis " << result.toString();
    if (v->verify(result, &counter_example)) {
        return result;
    }
    example_list.push_back(counter_example);
    io_example_list.push_back(io_space->getIOExample(counter_example));
    ProgramList term_list, condition_list;
    auto* env = spec->env.get();

    /*// Init Term List
    ProgramList params;
    for (int i = 0; i < 7; ++i) params.push_back(program::buildParam(i, theory::clia::getTInt()));
    term_list.push_back(params[0]);
    term_list.push_back(params[4]);
    term_list.push_back(std::make_shared<Program>(env->getSemantics("+"), (ProgramList){
        std::make_shared<Program>(env->getSemantics("+"), (ProgramList){params[1], params[5]}),
        program::buildConst(BuildData(Int, 1))
    }));
    term_list.push_back(std::make_shared<Program>(env->getSemantics("+"), (ProgramList){
            std::make_shared<Program>(env->getSemantics("+"), (ProgramList){params[3], params[2]}),
            program::buildConst(BuildData(Int, -1))
    }));
    term_list.push_back(std::make_shared<Program>(env->getSemantics("+"), (ProgramList){
            std::make_shared<Program>(env->getSemantics("+"), (ProgramList){params[3], params[6]}),
            program::buildConst(BuildData(Int, -1))
    }));
    for (int i = 0; i < 4; ++i) condition_list.push_back(program::buildConst(BuildData(Bool, true)));
    for (const auto&p: term_list) LOG(INFO) << "term " << p->toString();
    LOG(INFO) << example::ioExample2String(io_example_list[io_example_list.size() - 1]);*/

    while (true) {
        TimeCheck(guard);
        auto last_example = example_list[example_list.size() - 1];
        auto last_io_example = io_example_list[example_list.size() - 1];
        bool is_occur = false;
        for (const auto& term: term_list) {
            if (example::satisfyIOExample(term.get(), last_io_example, env)) {
                is_occur = true; break;
            }
        }
        if (!is_occur) {
            /*LOG(INFO) << "new term";*/
            auto new_term = term_solver->synthesisTerms(example_list, guard);
            //for (const auto& p: new_term) std::cout << "  " << p->toString() << std::endl;

            ProgramList new_condition;
            for (int id = 0; id + 1 < new_term.size(); ++id) {
                auto term = new_term[id];
                PProgram cond;
                for (int i = 0; i + 1 < term_list.size(); ++i) {
                    if (term->toString() == term_list[i]->toString()) {
                        cond = condition_list[i];
                        break;
                    }
                }
                if (!cond) {
                    cond = program::buildConst(BuildData(Bool, true));
                }
                new_condition.push_back(cond);
            }
            term_list = new_term;
            condition_list = new_condition;
        }
        IOExampleList rem_example = io_example_list;
        for (int i = 0; i + 1 < term_list.size(); ++i) {
            IOExampleList positive_list, negative_list, mid_list;
            auto current_term = term_list[i];
            for (const auto& current_example: rem_example) {
                if (example::satisfyIOExample(current_term.get(), current_example, env)) {
                    bool is_mid = false;
                    for (int j = i + 1; j < term_list.size(); ++j) {
                        if (example::satisfyIOExample(term_list[j].get(), current_example, env)) {
                            is_mid = true;
                            break;
                        }
                    }
                    if (is_mid) {
                        mid_list.push_back(current_example);
                    } else positive_list.push_back(current_example);
                } else negative_list.push_back(current_example);
            }

            auto condition = condition_list[i];
            bool is_valid = true;
            for (const auto& example: positive_list) {
                try {
                    if (!env->run(condition.get(), example.first).isTrue()) {
                        is_valid = false;
                        break;
                    }
                } catch (SemanticsError& e) {
                    is_valid = false;
                }
            }
            if (is_valid) {
                try {
                    for (const auto &example: negative_list) {
                        if (env->run(condition.get(), example.first).isTrue()) {
                            is_valid = false;
                            break;
                        }
                    }
                } catch (SemanticsError& e) {
                    is_valid = false;
                }
            }

            if (!is_valid) {
                condition = cond_solver->getCondition(term_list, positive_list, negative_list, guard);
            }
            condition = _handleSemanticsErrorInMid(condition, mid_list, env);

            condition_list[i] = condition;
            rem_example = negative_list;
            for (const auto& example: mid_list) {
                if (!env->run(condition.get(), example.first).isTrue()) {
                    rem_example.push_back(example);
                }
            }
        }

        auto merge = _mergeIte(term_list, condition_list, spec->env.get());
        result = semantics::buildSingleContext(info->name, merge);
        if (v->verify(result, &counter_example)) {
            return result;
        }
        // LOG(INFO) << "Counter Example " << example::ioExample2String(io_space->getIOExample(counter_example));
        example_list.push_back(counter_example);
        io_example_list.push_back(io_space->getIOExample(counter_example));
    }
}