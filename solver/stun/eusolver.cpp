//
// Created by pro on 2022/1/4.
//

#include "istool/solver/stun/eusolver.h"
#include "istool/solver/enum/enum.h"
#include "istool/basic/bitset.h"
#include "glog/logging.h"
#include <unordered_set>

EuTermSolver::EuTermSolver(const PSynthInfo &_tg, ExampleSpace *_example_space): tg(_tg), example_space(_example_space) {
}
EuUnifier::EuUnifier(const PSynthInfo &_cg, ExampleSpace *_example_space): cg(_cg), example_space(_example_space) {
    auto* io_space = dynamic_cast<IOExampleSpace*>(_example_space);
    if (!io_space) LOG(FATAL) << "EuUnifier supports only IOExampleSpace";
}
EuSolver::EuSolver(Specification *_spec, const PSynthInfo &tg, const PSynthInfo &cg):
    STUNSolver(_spec, new EuTermSolver(tg, _spec->example_space), new EuUnifier(cg, _spec->example_space)) {
}

namespace {
    class EuVerifier: public Verifier {
    public:
        ExampleList example_list;
        ExampleSpace* example_space;
        std::unordered_set<std::string> feature_set;
        std::vector<bool> is_satisfied;
        ProgramList term_list;
        EuVerifier(const ExampleList& _example_list, ExampleSpace* _example_space):
            example_list(_example_list), example_space(_example_space), is_satisfied(_example_list.size(), false) {
        }
        virtual bool verify(const FunctionContext& info, Example* counter_example) {
            if (counter_example) {
                LOG(INFO) << "EuVerifier is a dummy verifier for EuSolver. It cannot generate counterexamples.";
            }
            assert(info.size() == 1);
            bool is_finished = true;
            std::string feature;
            for (int i = 0; i < example_list.size(); ++i) {
                bool flag = example_space->satisfyExample(info, example_list[i]);
                feature += flag ? '1' : '0';
                if (!is_satisfied[i]) {
                    if (flag) is_satisfied[i] = true; else is_finished = false;
                }
            }
            if (feature_set.find(feature) == feature_set.end()) {
                term_list.push_back(info.begin()->second);
            }
            if (is_finished) return true;
        }
    };
}

ProgramList EuTermSolver::synthesisTerms(const ExampleList &example_list, TimeGuard *guard) {
    auto* verifier = new EuVerifier(example_list, example_space);
    auto* optimizer = new TrivialOptimizer();

    EnumConfig c(verifier, optimizer, guard);
    auto enum_res = solver::enumerate({tg}, c);
    if (enum_res.empty()) {
        LOG(FATAL) << "EuTermSolver: Synthesis Failed";
    }
    auto res = verifier->term_list;
    delete verifier; delete optimizer;
    return res;
}

namespace {
    struct UnifyInfo {
        PProgram predicate;
        Bitset info;
        UnifyInfo(const PProgram& _pred, const Bitset& _info): predicate(_pred), info(_info) {
        }
    };

    class PredicateVerifier: public Verifier {
    public:
        ExampleList example_list;
        std::unordered_set<std::string> feature_set;
        std::vector<UnifyInfo> info_list;
        PredicateVerifier(const ExampleList& _example_list): example_list(_example_list) {}
        virtual bool verify(const FunctionContext& res, Example* counter_example) {
            if (counter_example) {
                LOG(INFO) << "PredicateVerifier is a dummy verifier for EuSolver. It cannot generate counterexamples.";
            }
            assert(res.size() == 1); auto prog = res.begin()->second;
            Bitset info;
            for (int i = 0; i < example_list.size(); ++i) {
                info.append(program::run(prog.get(), example_list[i]).isTrue());
            }

            auto feature = info.toString();
            if (feature_set.find(feature) == feature_set.end()) {
                info_list.emplace_back(prog, info);
            }
        }
    };

    class DTLearner {
    public:
        std::vector<UnifyInfo> pred_info_list, term_info_list;
        int n;
        DTLearner(const std::vector<UnifyInfo>& _info_list, const std::vector<UnifyInfo>& _term_list):
            pred_info_list(_info_list), term_info_list(_term_list), n(term_info_list[0].info.size()) {
        }
        PProgram learn() {
            std::vector<int> pts;
            for (int i = 0; i < n; ++i) pts.push_back(i);
            std::vector<int> preds;
            for (int i = 0; i < info_list.size(); ++i) preds.push_back(i);

        }
        ~DTLearner() = default;
    };
}

PProgram EuUnifier::unify(const ProgramList &term_list, const ExampleList &example_list, TimeGuard *guard) {
    if (term_list.size() == 1) return term_list[1];
    auto* verifier = new PredicateVerifier(example_list);
    auto* optimizer = new TrivialOptimizer();
    EnumConfig c(verifier, optimizer, guard);

    while (1) {
        auto enum_res = solver::enumerate({cg}, c);
        if (enum_res.empty()) LOG(FATAL) << "EuUnifier: synthesis failed";

    }

}