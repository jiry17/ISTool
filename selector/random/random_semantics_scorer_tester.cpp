//
// Created by pro on 2022/4/30.
//

#include "istool/selector/random/random_semantics_scorer_tester.h"
#include "glog/logging.h"

namespace {
    struct _ProgramInfo {
        double prob;
        PProgram program;
        _ProgramInfo(double _prob, const PProgram& _program): prob(_prob), program(_program) {}
    };

    typedef std::vector<_ProgramInfo> _ProgramInfoList;
    typedef std::vector<_ProgramInfoList> _ProgramInfoStorage;

    void _mergeAllProgramInfo(int pos, double prob, ProgramList& tmp, const PSemantics& sem, const _ProgramInfoStorage& storage, _ProgramInfoList& res) {
        if (pos == storage.size()) {
            res.emplace_back(prob, std::make_shared<Program>(sem, tmp));
            return;
        }
        for (auto& info: storage[pos]) {
            tmp[pos] = info.program;
            _mergeAllProgramInfo(pos + 1, prob * info.prob, tmp, sem, storage, res);
        }
    }

    _ProgramInfoList _mergeAllProgramInfo(double weight, const PSemantics& sem, const _ProgramInfoStorage& storage) {
        _ProgramInfoList res;
        ProgramList tmp(storage.size());
        _mergeAllProgramInfo(0, weight, tmp, sem, storage, res);
        return res;
    }

    _ProgramInfoList _collectAllProgramInfo(int node_id, TopDownContextGraph* graph, std::unordered_map<int, _ProgramInfoList>& cache) {
        if (cache.count(node_id)) return cache[node_id];
        _ProgramInfoList res;
        auto& node = graph->node_list[node_id];
        for (auto& edge: node.edge_list) {
            _ProgramInfoStorage storage(edge.v_list.size());
            for (int i = 0; i < edge.v_list.size(); ++i) {
                storage[i] = _collectAllProgramInfo(edge.v_list[i], graph, cache);
            }
            for (auto& merged_info: _mergeAllProgramInfo(edge.weight, edge.semantics, storage)) {
                res.push_back(merged_info);
            }
        }
        return cache[node_id] = res;
    }

    _ProgramInfoList _collectAllProgramInfo(TopDownContextGraph* graph) {
        std::unordered_map<int, _ProgramInfoList> cache;
        _collectAllProgramInfo(0, graph, cache);
        return cache[0];
    }

    bool _isTerminate(TopDownGraphMatchStructure* s) {
        auto* sem = s->program->semantics.get();
        return dynamic_cast<ParamSemantics*>(sem) || dynamic_cast<ConstSemantics*>(sem);
    }

    Data _getTerminateOutput(TopDownGraphMatchStructure* s, const DataList& inp_list) {
        auto* sem = s->program->semantics.get();
        auto* cs = dynamic_cast<ConstSemantics*>(sem);
        if (cs) return cs->w;
        auto* ps = dynamic_cast<ParamSemantics*>(sem);
        return inp_list[ps->id];
    }

    double _getRandomMatchProb(TopDownGraphMatchStructure* fs, TopDownGraphMatchStructure* gs, const DataList& inp_list, double KO) {
        int terminate_count = _isTerminate(fs) + _isTerminate(gs);
        if (terminate_count == 1) return 1 / KO;
        if (terminate_count == 2) {
            auto f_res = _getTerminateOutput(fs, inp_list), g_res = _getTerminateOutput(gs, inp_list);
            if (f_res == g_res) return 1.0; else return 0.0;
        }
        if (fs->edge_id != gs->edge_id) return 1 / KO;
        double equal_prob = 1.0;
        for (int i = 0; i < fs->sub_list.size(); ++i) {
            equal_prob *= _getRandomMatchProb(fs->sub_list[i], gs->sub_list[i], inp_list, KO);
        }
        return equal_prob + (1 - equal_prob) / KO;
    }

    double _getRandomMatchProb(std::vector<TopDownGraphMatchStructure*> s_list, const DataList& inp_list, double KO) {
        assert(s_list.size() == 3);
        std::sort(s_list.begin(), s_list.end(), [](TopDownGraphMatchStructure* x, TopDownGraphMatchStructure* y) {
            bool xf = _isTerminate(x), yf = _isTerminate(y);
            if (xf && (!yf)) return true; if ((!xf) && yf) return false;
            return x->edge_id < y->edge_id;
        });
        if (_isTerminate(s_list[2])) {
            DataList oup_list(3);
            for (int i = 0; i < 3; ++i) oup_list[i] = _getTerminateOutput(s_list[i], inp_list);
            if (oup_list[0] == oup_list[1] && oup_list[1] == oup_list[2]) return 1.0;
            return 0.0;
        }
        if (_isTerminate(s_list[1])) {
            if (_getTerminateOutput(s_list[0], inp_list) == _getTerminateOutput(s_list[1], inp_list))
                return 1.0 / KO;
            return 0.0;
        }
        if (s_list[0]->edge_id == s_list[1]->edge_id && s_list[1]->edge_id == s_list[2]->edge_id) {
            double all_prob = 1.0;
            double x[3];
            for (int i = 0; i < 3; ++i) x[i] = 1.0;
            for (int i = 0; i < s_list[0]->sub_list.size(); ++i) {
                std::vector<TopDownGraphMatchStructure*> sub_list(3);
                for (int j = 0; j < 3; ++j) sub_list[j] = s_list[j]->sub_list[i];
                all_prob *= _getRandomMatchProb(sub_list, inp_list, KO);
                for (int j = 0; j < 3; ++j) {
                    x[j] *= _getRandomMatchProb(sub_list[j], sub_list[(j + 1) % 3], inp_list, KO);
                }
            }
            double rem_prob = 1.0 - all_prob, res = all_prob;
            for (int j = 0; j < 3; ++j) {
                double now = x[j] - all_prob;
                rem_prob -= now; res += now / KO;
            }
            return res + rem_prob / KO / KO;
        }
        if (s_list[1]->edge_id == s_list[2]->edge_id) std::swap(s_list[0], s_list[2]);
        if (s_list[0]->edge_id == s_list[1]->edge_id) {
            return _getRandomMatchProb(s_list[0], s_list[1], inp_list, KO) / KO;
        }
        return 1.0 / KO / KO;
    }

    DataStorage _getFullInpList(Env* env, const DataStorage& raw_inp, const std::vector<std::pair<PType, PProgram>>& param_list) {
        DataStorage res;
        for (auto& inp: raw_inp) {
            DataList new_inp;
            for (auto& param_info: param_list) {
                new_inp.push_back(env->run(param_info.second.get(), inp));
            }
            res.push_back(new_inp);
        }
        return res;
    }
}

double selector::test::getPairGroundTruth(Env* env, FlattenGrammar *fg, const DataStorage &raw_inp, double KO) {
    auto info_list = _collectAllProgramInfo(fg->graph);
    /*LOG(INFO) << "Program Infos:";
    for (auto& info: info_list) {
        LOG(INFO) << "  " << info.prob << ": " << info.program->toString() << std::endl;
    }*/
    auto inp_list = _getFullInpList(env, raw_inp, fg->param_list);
    double res = 0.0;
    for (auto& f_info: info_list) {
        auto* fs = fg->graph->getProgramMatchStructure(f_info.program); assert(fs);
        for (auto& g_info: info_list) {
            auto* gs = fg->graph->getProgramMatchStructure(g_info.program); assert(gs);
            double cur = 1.0;
            for (auto& inp: inp_list) cur *= _getRandomMatchProb(fs, gs, inp, KO);
            res += cur * f_info.prob * g_info.prob;
            delete gs;
        }
        delete fs;
    }
    return res;
}

double selector::test::getTripleGroundTruth(Env* env, FlattenGrammar *fg, const PProgram& p, const DataStorage &raw_inp, double KO) {
    _ProgramInfoList info_list = _collectAllProgramInfo(fg->graph);
    LOG(INFO) << "prog size " << info_list.size();
    /*LOG(INFO) << "Program Infos:";
    for (auto& info: info_list) {
        LOG(INFO) << "  " << info.prob << ": " << info.program->toString() << std::endl;
    }*/
    auto inp_list = _getFullInpList(env, raw_inp, fg->param_list);
    auto* ps = fg->getMatchStructure(p); assert(ps);
    double res = 0.0;
    for (auto& f_info: info_list) {
        auto* fs = fg->graph->getProgramMatchStructure(f_info.program); assert(fs);
        for (auto& g_info: info_list) {
            auto* gs = fg->graph->getProgramMatchStructure(g_info.program); assert(gs);
            double cur = _getRandomMatchProb(fs, gs, inp_list[inp_list.size() - 1], KO);
            for (int i = 0; i + 1 < inp_list.size(); ++i)
                cur *= _getRandomMatchProb({fs, gs, ps}, inp_list[i], KO);
            // std::cout << f_info.program->toString() << " " << g_info.program->toString() << " " << cur << std::endl;
            res += cur * f_info.prob * g_info.prob;
            delete gs;
        }
        delete fs;
    }
    delete ps;
    return res;
}
