//
// Created by pro on 2022/5/2.
//

#include "istool/selector/random/grammar_flatter.h"
#include <unordered_set>
#include "glog/logging.h"

FlattenGrammar::ParamInfo::ParamInfo(const PType &_type, const PProgram &_program): type(_type), program(_program) {
}

FlattenGrammar::FlattenGrammar(TopDownContextGraph *_graph, Env* _env): graph(_graph), env(_env) {
}
FlattenGrammar::~FlattenGrammar() {
    delete graph;
    for (auto& info: match_cache) delete info.second;
}
void FlattenGrammar::print() const {
    graph->print();
    for (int i = 0; i < param_info_list.size(); ++i) {
        std::cout << "  #" << i << ": " << param_info_list[i].program->toString() << std::endl;
    }
}
TopDownGraphMatchStructure * FlattenGrammar::getMatchStructure(const PProgram &program) {
    auto feature = program->toString();
    if (match_cache.count(feature)) return match_cache[feature];
    auto res = getMatchStructure(0, program);
    if (!res) {
        LOG(FATAL) << "Cannot match program " << program->toString();
    }
    return res;
}
Example FlattenGrammar::getFlattenInput(const Example &input) const {
    Example new_input;
    for (auto& param: param_info_list) new_input.push_back(env->run(param.program.get(), input));
    return new_input;
}
typedef std::function<bool(Program*)> ProgramValidator;

namespace {
    typedef std::pair<PProgram, double> ProgramInfo;
    bool _isTerminateSemantics(Semantics* sem) {
        return dynamic_cast<ParamSemantics*>(sem) || dynamic_cast<ConstSemantics*>(sem);
    }
    int _getTerminateSize(Semantics* sem, const std::vector<int>& param_size_list) {
        auto* ps = dynamic_cast<ParamSemantics*>(sem);
        if (ps) return param_size_list[ps->id];
        assert(dynamic_cast<ConstSemantics*>(sem));
        return 1;
    }
    void _collectAllProgram(int pos, double prob, ProgramList& tmp, const std::vector<std::vector<ProgramInfo>>& storage, std::vector<ProgramInfo>& res, const PSemantics& sem) {
        if (pos == storage.size()) {
            res.emplace_back(std::make_shared<Program>(sem, tmp), prob); return;
        }
        for (auto& info: storage[pos]) {
            tmp[pos] = info.first;
            _collectAllProgram(pos + 1, prob * info.second, tmp, storage, res, sem);
        }
    }
    std::vector<ProgramInfo> _collectAllProgram(const std::vector<std::vector<ProgramInfo>>& storage, const TopDownContextGraph::Edge& edge) {
        ProgramList tmp(storage.size());
        std::vector<ProgramInfo> res;
        _collectAllProgram(0, edge.weight, tmp, storage, res, edge.semantics);
        return res;
    }

    void _deleteEmptyNode(TopDownContextGraph* graph) {
        while (1) {
            int node_id = -1;
            for (int i = 0; i < graph->node_list.size(); ++i) {
                if (graph->node_list[i].edge_list.empty()) {
                    node_id = i; break;
                }
            }
            if (node_id == -1) return;
            for (int i = node_id; i + 1 < graph->node_list.size(); ++i) {
                graph->node_list[i] = graph->node_list[i + 1];
            }
            graph->node_list.pop_back();
            for (auto& node: graph->node_list) {
                int now = 0;
                for (auto& edge: node.edge_list) {
                    bool is_removed = false;
                    for (auto& v: edge.v_list) {
                        if (v == node_id) {
                            is_removed = true; break;
                        } else if (v > node_id) --v;
                    }
                    if (!is_removed) node.edge_list[now++] = edge;
                }
                while (node.edge_list.size() > now) node.edge_list.pop_back();
            }
        }
    }

    const int KSizeLimit = 1e9;

    std::pair<int, int> _getBestMergePos(TopDownContextGraph* g, const std::vector<int>& param_size_list) {
        std::vector<bool> is_final(g->node_list.size(), false);
        std::vector<double> average_size_list(g->node_list.size(), 0.0);
        for (int node_id = 0; node_id < g->node_list.size(); ++node_id) {
            auto& node = g->node_list[node_id];
            bool flag = true;
            for (auto& edge: node.edge_list) {
                if (!_isTerminateSemantics(edge.semantics.get())) {
                    flag = false;
                    auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
                    if (ps) average_size_list[node_id] += param_size_list[ps->id] * edge.weight;
                    else average_size_list[node_id] += edge.weight;
                }
            }
            if (flag) is_final[node_id] = true;
        }
        int best_node = -1, best_edge = -1;
        double best_prob = 1e9;
        int best_size = KSizeLimit;
        for (int node_id = 0; node_id < g->node_list.size(); ++node_id) {
            if (is_final[node_id]) continue;
            auto& node = g->node_list[node_id];
            for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
                auto& edge = node.edge_list[edge_id];
                if (_isTerminateSemantics(edge.semantics.get())) continue;
                double average_size = 1.0; bool flag = true;
                for (auto sub_id: edge.v_list) {
                    if (!is_final[sub_id]) {
                        flag = false; break;
                    }
                    average_size += average_size_list[sub_id];
                }
                if (average_size < best_size && flag) {
                    best_size = average_size; best_node = node_id; best_edge = edge_id;
                }
            }
        }
        return {best_node, best_edge};
    }

    const int KExtraFactor = 20;
}


TopDownGraphMatchStructure * TrivialFlattenGrammar::getMatchStructure(int node_id, const PProgram& p) const {
    auto feature = p->toString();
    auto it = param_map.find(feature);
    auto& node = graph->node_list[node_id];
    if (it != param_map.end()) {
        int param_id = it->second;
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            auto *ps = dynamic_cast<ParamSemantics*>(node.edge_list[edge_id].semantics.get());
            if (ps && ps->id == param_id) return new TopDownGraphMatchStructure(edge_id, program::buildParam(param_id, param_info_list[param_id].type), {});
        }
    }
    auto sem_name = p->semantics->getName();
    for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
        auto& edge = node.edge_list[edge_id];
        if (edge.semantics->getName() == sem_name) {
            std::vector<TopDownGraphMatchStructure*> sub_list;
            bool flag = true;
            for (int i = 0; i < p->sub_list.size(); ++i) {
                auto* sub_match = getMatchStructure(edge.v_list[i], p->sub_list[i]);
                if (!sub_match) {
                    flag = false; break;
                }
                sub_list.push_back(sub_match);
            }
            if (flag) return new TopDownGraphMatchStructure(edge_id, p, sub_list);
            for (auto* sub_match: sub_list) delete sub_match;
        }
    }
    return nullptr;
}


namespace {
    std::vector<ProgramInfo> _getAllProgramsFromEdge(TopDownContextGraph* g, const TopDownContextGraph::Edge& edge,
            int limit, const std::vector<FlattenGrammar::ParamInfo>& param_info_list) {
        std::vector<std::vector<ProgramInfo>> storage;
        int approx_num = 1;
        for (auto sub_id: edge.v_list) {
            std::vector<ProgramInfo> info_list;
            for (auto& sub_edge: g->node_list[sub_id].edge_list) {
                assert(_isTerminateSemantics(sub_edge.semantics.get()));
                if (dynamic_cast<ConstSemantics*>(sub_edge.semantics.get())) {
                    info_list.emplace_back(std::make_shared<Program>(sub_edge.semantics, (ProgramList){}), sub_edge.weight);
                } else {
                    auto* ps = dynamic_cast<ParamSemantics*>(sub_edge.semantics.get());
                    assert(ps);
                    info_list.emplace_back(param_info_list[ps->id].program, sub_edge.weight);
                }
            }
            approx_num = std::min(1ll * approx_num * int(info_list.size()), 1ll * limit + 1);
            storage.push_back(info_list);
        }
        if (approx_num > limit) return {};
        return _collectAllProgram(storage, edge);
    }
}

TrivialFlattenGrammar::TrivialFlattenGrammar(TopDownContextGraph *_g, Env *_env, int flatten_num, const std::function<bool(Program*)>& validator): FlattenGrammar(_g, _env) {
    assert(_g->prob_type == ProbModelType::NORMAL_PROB);
    for (auto& node: graph->node_list) {
        for (auto& edge: node.edge_list) {
            auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
            if (ps) {
                while (param_info_list.size() <= ps->id) param_info_list.emplace_back();
                if (ps->oup_type) {
                    auto pp = program::buildParam(ps->id, ps->oup_type);
                    param_info_list[ps->id] = {ps->oup_type, pp};
                }
            }
        }
    }

    std::vector<int> param_size_list;
    for (int i = 0; i < param_info_list.size(); ++i) {
        param_map[param_info_list[i].program->toString()] = i;
    }
    while (1) {
        for (int i = param_size_list.size(); i < param_info_list.size(); ++i) {
            param_size_list.push_back(param_info_list[i].program->size());
        }
        auto best_pos = _getBestMergePos(graph, param_size_list);
        int best_node = best_pos.first, best_edge = best_pos.second;
        if (best_node == -1) break;

        auto& node = graph->node_list[best_node];
        auto edge = node.edge_list[best_edge];
        auto info_list = _getAllProgramsFromEdge(graph, edge, KExtraFactor * flatten_num, param_info_list);
        int now = 0;
        for (auto& info: info_list) {
            if (!validator(info.first.get())) continue;
            info_list[now++] = info;
            auto feature = info.first->toString();
            if (param_map.count(feature) == 0) {
                if (param_map.size() > flatten_num) break;
                param_map[feature] = -1;
            }
        }
        info_list.resize(now);
        if (param_map.size() > flatten_num) break;

        for (int i = best_edge + 1; i < node.edge_list.size(); ++i) {
            node.edge_list[i - 1] = node.edge_list[i];
        }
        node.edge_list.pop_back();
        for (auto& p_info: info_list) {
            auto feature = p_info.first->toString();
            assert(param_map.count(feature));
            int id = param_map[feature];
            if (id == -1) {
                id = param_info_list.size();
                param_info_list.emplace_back(node.symbol->type, p_info.first);
                param_map[feature] = id;
            }
            auto sem = semantics::buildParamSemantics(id, param_info_list[id].type);
            node.edge_list.emplace_back(best_node, (std::vector<int>){}, p_info.second, sem);
        }
        if (node.edge_list.empty()) _deleteEmptyNode(graph);
    }

    // init prob and param_map
    param_map.clear();
    for (int i = 0; i < param_info_list.size(); ++i) {
        param_map[param_info_list[i].program->toString()] = i;
    }
    graph->normalizeProbability();
}

namespace {
    class _FiniteEquivalenceCheckerTool: public EquivalenceCheckTool {
    public:
        DataStorage inp_pool;
        Env* env;
        std::unordered_map<std::string, PProgram> feature_map;
        std::string getFeature(Program* p) {
            DataList res;
            for (const auto& inp: inp_pool) res.push_back(env->run(p, inp));
            return data::dataList2String(res);
        }
        _FiniteEquivalenceCheckerTool(Env* _env, FiniteIOExampleSpace* fio): env(_env) {
            for (auto& example: fio->example_space) {
                auto io_example = fio->getIOExample(example);
                inp_pool.push_back(io_example.first);
            }
        }
        virtual PProgram insertProgram(const PProgram& p) {
            auto feature = getFeature(p.get());
            if (feature_map.count(feature) == 0) feature_map[feature] = p;
            return feature_map[feature];
        }
        virtual PProgram queryProgram(const PProgram& p) {
            auto feature = getFeature(p.get());
            if (feature_map.count(feature) == 0) return {};
            return feature_map[feature];
        }
        virtual Data getConst(Program* p) {
            auto res = env->run(p, inp_pool[0]);
            for (auto& inp: inp_pool) {
                auto now = env->run(p, inp);
                if (!(now == res)) return {};
            }
            return res;
        }
    };

    void _mergeSameEdge(TopDownContextGraph::Node& node) {
        std::unordered_map<int, int> param_pos;
        std::unordered_map<std::string, int> const_pos;
        int now = 0;
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            auto& edge = node.edge_list[edge_id];
            auto* cs = dynamic_cast<ConstSemantics*>(edge.semantics.get());
            if (cs) {
                auto feature = cs->w.toString();
                if (const_pos.count(feature)) {
                    node.edge_list[const_pos[feature]].weight += edge.weight;
                } else {
                    const_pos[feature] = now;
                    node.edge_list[now++] = edge;
                }
                continue;
            }
            auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
            if (ps) {
                if (param_pos.count(ps->id)) {
                    node.edge_list[param_pos[ps->id]].weight += edge.weight;
                } else {
                    param_pos[ps->id] = now;
                    node.edge_list[now++] = edge;
                }
                continue;
            }
            node.edge_list[now++] = edge;
        }
        while (node.edge_list.size() > now) node.edge_list.pop_back();
    }
}

MergedFlattenGrammar::MergedFlattenGrammar(TopDownContextGraph *_g, Env *env, int flatten_num, const std::function<bool (Program *)> &validator, ExampleSpace *example_space):
    FlattenGrammar(_g, env), tool(nullptr) {
    auto* fio = dynamic_cast<FiniteIOExampleSpace*>(example_space);
    if (fio) {
        tool = new _FiniteEquivalenceCheckerTool(env, fio);
    } else assert(false);

    assert(_g->prob_type == ProbModelType::NORMAL_PROB);
    for (auto& node: graph->node_list) {
        for (auto& edge: node.edge_list) {
            auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
            if (ps) {
                while (param_info_list.size() <= ps->id) param_info_list.emplace_back();
                if (ps->oup_type) {
                    auto pp = program::buildParam(ps->id, ps->oup_type);
                    param_info_list[ps->id] = {ps->oup_type, pp};
                }
            }
        }
    }

    std::vector<int> param_size_list;
    for (int i = 0; i < param_info_list.size(); ++i) {
        param_map[param_info_list[i].program->toString()] = i;
        tool->insertProgram(param_info_list[i].program);
    }
    while (1) {
        for (int i = param_size_list.size(); i < param_info_list.size(); ++i) {
            param_size_list.push_back(param_info_list[i].program->size());
        }
        auto best_pos = _getBestMergePos(graph, param_size_list);
        int best_node = best_pos.first, best_edge = best_pos.second;
        if (best_node == -1) break;

        auto& node = graph->node_list[best_node];
        auto edge = node.edge_list[best_edge];
        auto info_list = _getAllProgramsFromEdge(graph, edge, KExtraFactor * flatten_num, param_info_list);
        std::cout << best_node << " " << best_edge << " " << info_list.size() << std::endl;
        if (info_list.empty()) break;
        int now = 0;
        for (auto& info: info_list) {
            if (!validator(info.first.get())) continue;
            info_list[now++] = info;
            if (tool->getConst(info.first.get()).isNull()) {
                auto feature = tool->insertProgram(info.first)->toString();
                if (param_map.count(feature) == 0) param_map[feature] = -1;
            }
        }
        info_list.resize(now);
        if (param_map.size() > flatten_num) break;

        for (int i = best_edge + 1; i < node.edge_list.size(); ++i) {
            node.edge_list[i - 1] = node.edge_list[i];
        }
        node.edge_list.pop_back();
        for (auto& p_info: info_list) {
            auto cons = tool->getConst(p_info.first.get());
            if (!cons.isNull()) {
              //  LOG(INFO) << "skip " << p_info.first->toString() << " " << cons.toString() << std::endl;
                auto sem = semantics::buildConstSemantics(cons);
                node.edge_list.emplace_back(best_node, (std::vector<int>){}, p_info.second, sem);
                continue;
            }
            auto feature = tool->insertProgram(p_info.first)->toString();
//            if (feature != p_info.first->toString()) LOG(INFO) << "merge " << p_info.first->toString() << " " << feature << std::endl;
            assert(param_map.count(feature));
            int id = param_map[feature];
            if (id == -1) {
                id = param_info_list.size();
                param_info_list.emplace_back(node.symbol->type, p_info.first);
                param_map[feature] = id;
            }
            auto sem = semantics::buildParamSemantics(id, param_info_list[id].type);
            node.edge_list.emplace_back(best_node, (std::vector<int>){}, p_info.second, sem);
        }
        _mergeSameEdge(node);
        if (node.edge_list.empty()) _deleteEmptyNode(graph);
    }

    // init prob and param_map
    param_map.clear();
    for (int i = 0; i < param_info_list.size(); ++i) {
        param_map[param_info_list[i].program->toString()] = i;
    }
    graph->normalizeProbability();
}

TopDownGraphMatchStructure * MergedFlattenGrammar::getMatchStructure(int node_id, const PProgram &program) const {
    auto cons = tool->getConst(program.get());
    auto& node = graph->node_list[node_id];
    if (!cons.isNull()) {
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            auto *cs = dynamic_cast<ConstSemantics *>(node.edge_list[edge_id].semantics.get());
            if (cs && cs->w == cons) return new TopDownGraphMatchStructure(edge_id, program::buildConst(cons), {});
        }
    }
    auto param_prog = tool->queryProgram(program);
    if (param_prog) {
        auto feature = param_prog->toString(); // std::cout << "param " << program->toString() << " " << feature << std::endl;
        auto it = param_map.find(feature);
        if (it != param_map.end()) {
            int param_id = it->second;
            for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
                auto *ps = dynamic_cast<ParamSemantics *>(node.edge_list[edge_id].semantics.get());
                if (ps && ps->id == param_id)
                    return new TopDownGraphMatchStructure(edge_id,
                                                          program::buildParam(param_id, param_info_list[param_id].type), {});
            }
        }
    }
    auto sem_name = program->semantics->getName();
    for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
        auto& edge = node.edge_list[edge_id];
        if (edge.semantics->getName() == sem_name) {
            std::vector<TopDownGraphMatchStructure*> sub_list;
            bool flag = true;
            for (int i = 0; i < program->sub_list.size(); ++i) {
                auto* sub_match = getMatchStructure(edge.v_list[i], program->sub_list[i]);
                if (!sub_match) {
                    flag = false; break;
                }
                sub_list.push_back(sub_match);
            }
            if (flag) return new TopDownGraphMatchStructure(edge_id, program, sub_list);
            for (auto* sub_match: sub_list) delete sub_match;
        }
    }
    LOG(INFO) << "fail in match " << program->toString() << std::endl;
    return nullptr;
}
MergedFlattenGrammar::~MergedFlattenGrammar() {
    delete tool;
}