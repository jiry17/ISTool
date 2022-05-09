//
// Created by pro on 2022/5/3.
//

#include "istool/selector/random/random_semantics_selector.h"
#include "istool/selector/random/program_adaptor.h"
#include "glog/logging.h"
#include <iomanip>

BasicRandomSemanticsSelector::BasicRandomSemanticsSelector(Env* _env, Grammar* _g, RandomSemanticsScorer *_scorer): env(_env), g(_g), scorer(_scorer) {}
BasicRandomSemanticsSelector::~BasicRandomSemanticsSelector() noexcept {
    delete scorer;
}
namespace {
    const int KMaxExampleNum = 9;
}
void BasicRandomSemanticsSelector::addHistoryExample(const Example &inp) {
    if (history_inp_list.size() == KMaxExampleNum) {
        for (int i = 1; i < history_inp_list.size(); ++i) history_inp_list[i - 1] = history_inp_list[i];
        history_inp_list[KMaxExampleNum - 1] = inp;
    } else history_inp_list.push_back(inp);
}
int BasicRandomSemanticsSelector::getBestExampleId(const PProgram& program, const ExampleList &candidate_list) {
    int best_id = -1; double best_cost = 1e9;
    auto p = selector::adaptor::programAdaptorWithLIARules(program.get(), g);
#ifdef DEBUG
    assert(p);
    if (p->toString() != program->toString()) {
        LOG(INFO) << "Program adaption: " << program->toString() << " --> " << p->toString();
        for (auto& inp: candidate_list) {
            assert(env->run(p.get(), inp) == env->run(program.get(), inp));
        }
    }
#endif
    for (int id = 0; id < candidate_list.size(); ++id) {
        auto& inp = candidate_list[id];
        history_inp_list.push_back(inp);
        double cost = scorer->getTripleScore(p, history_inp_list);
        std::cout << std::setprecision(15) << data::dataList2String(inp) << " " << cost << std::endl;
        history_inp_list.pop_back();
        if (cost < best_cost) {
            best_cost = cost; best_id = id;
        }
    }
    return best_id;
}

FiniteRandomSemanticsSelector::FiniteRandomSemanticsSelector(Specification *spec, RandomSemanticsScorer *_scorer):
    BasicRandomSemanticsSelector(spec->env.get(), spec->info_list[0]->grammar, _scorer) {
    io_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (!io_space) {
        LOG(FATAL) << "FiniteRandomSemanticsSelector supports only FiniteIOExampleSpace";
    }
    for (auto& example: io_space->example_space) {
        io_example_list.push_back(io_space->getIOExample(example));
    }
}
bool FiniteRandomSemanticsSelector::verify(const FunctionContext &info, Example *counter_example) {
    auto p = info.begin()->second;
    std::vector<int> counter_id_list;
    ExampleList candidate_inp_list;
    for (int i = 0; i < io_example_list.size(); ++i) {
        auto& example = io_example_list[i];
        if (!(env->run(p.get(), example.first) == example.second)) {
            counter_id_list.push_back(i);
            candidate_inp_list.push_back(example.first);
            if (!counter_example) return false;
        }
    }
    if (candidate_inp_list.empty()) return true;
    int best_id = counter_id_list[getBestExampleId(p, candidate_inp_list)];
    *counter_example = io_space->example_space[best_id];
    addHistoryExample(io_example_list[best_id].first);
    return false;
}

Z3RandomSemanticsSelector::Z3RandomSemanticsSelector(Specification *spec, RandomSemanticsScorer *_scorer, int _KExampleNum):
    Z3Verifier(dynamic_cast<Z3ExampleSpace*>(spec->example_space.get())), BasicRandomSemanticsSelector(spec->env.get(), spec->info_list[0]->grammar, _scorer), KExampleNum(_KExampleNum) {
    z3_io_space = dynamic_cast<Z3IOExampleSpace*>(spec->example_space.get());
    if (!z3_io_space) {
        LOG(FATAL) << "Z3RandomSemanticsSelector supports only Z3IOExampleSpace";
    }
}
bool Z3RandomSemanticsSelector::verify(const FunctionContext &info, Example *counter_example) {
    auto p = info.begin()->second;
    z3::solver s(ext->ctx);
    prepareZ3Solver(s, info);
    auto res = s.check();
    if (res == z3::unsat) return true;
    if (res != z3::sat) {
        LOG(FATAL) << "Z3 failed with " << res;
    }
    ExampleList example_list;
    Example example;
    auto model = s.get_model();
    getExample(model, &example);
    example_list.push_back(example);
    auto param_list = getParamVector();
    for (int _ = 1; _ < KExampleNum; ++_) {
        z3::expr_vector block(ext->ctx);
        for (int i = 0; i < param_list.size(); ++i) {
            block.push_back(ext->buildConst(example[i]) != param_list[i]);
        }
        s.add(z3::mk_or(block));
        auto status = s.check();
        if (status == z3::unsat) break;
        model = s.get_model(); getExample(model, &example);
        example_list.push_back(example);
    }

    ExampleList inp_list;
    for (auto& current_example: example_list) {
        inp_list.push_back(z3_io_space->getIOExample(current_example).first);
    }
    int best_id = getBestExampleId(p, inp_list);
    addHistoryExample(inp_list[best_id]);
    *counter_example = example_list[best_id];
    return false;
}

FiniteCompleteRandomSemanticsSelector::FiniteCompleteRandomSemanticsSelector(Specification *spec,
        EquivalenceChecker *checker, RandomSemanticsScorer *scorer, DifferentProgramGenerator* _g):
    CompleteSelector(spec, checker), BasicRandomSemanticsSelector(spec->env.get(), spec->info_list[0]->grammar, scorer), g(_g) {
    fio_space = dynamic_cast<FiniteIOExampleSpace*>(spec->example_space.get());
    if (!fio_space) {
        LOG(FATAL) << "FiniteCompleteRandomSemanticsSelector require FiniteIOExampleSpace";
    }
    for (auto& example: fio_space->example_space) {
        io_example_list.push_back(fio_space->getIOExample(example));
    }
}
FiniteCompleteRandomSemanticsSelector::~FiniteCompleteRandomSemanticsSelector() noexcept {
    delete g;
}
void FiniteCompleteRandomSemanticsSelector::addExample(const IOExample &example) {
    addHistoryExample(example.first);
    checker->addExample(example);
    g->addExample(example);
}
Example FiniteCompleteRandomSemanticsSelector::getNextExample(const PProgram &x, const PProgram &y) {
    LOG(INFO) << x->toString() << std::endl;
    std::vector<int> id_list;
    ExampleList candidate_inp_list;
    for ( int i = 0; i < io_example_list.size(); ++i) {
        auto& io_example = io_example_list[i];
        auto program_list = g->getDifferentProgram(io_example, 2);
        if (program_list.size() == 1) continue;
        id_list.push_back(i);
        candidate_inp_list.push_back(io_example.first);
    }
    int best_id = getBestExampleId(x, candidate_inp_list);
    best_id = id_list[best_id];
    auto res = fio_space->example_space[best_id];
#ifdef DEBUG
    auto res_io = fio_space->getIOExample(res);
    assert(!(env->run(x.get(), res_io.first) == env->run(y.get(), res_io.first)));
#endif
    return res;
}