//
// Created by pro on 2022/5/2.
//

#include "istool/selector/random/grammar_flatter.h"
#include "glog/logging.h"

FlattenGrammar::FlattenGrammar(const std::vector<std::pair<PType, PProgram> > &_param_list, TopDownContextGraph *_g):
    param_list(_param_list), graph(_g), is_indexed(false) {
}
FlattenGrammar::~FlattenGrammar() {
    delete graph;
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

    bool _oneStepFlatten(FlattenGrammar* fg, int size_limit, const ProgramValidator& validator) {
        auto* g = fg->graph;
        std::vector<int> param_size_list;
        for (auto& p: fg->param_list) {
            param_size_list.push_back(p.second->size());
        }
        std::vector<bool> is_final(g->node_list.size(), false);
        std::vector<double> average_list(g->node_list.size(), 0);
        for (int node_id = 0; node_id < g->node_list.size(); ++node_id) {
            auto& node = g->node_list[node_id];
            bool flag = true;
            for (auto& edge: node.edge_list) {
                if (!_isTerminateSemantics(edge.semantics.get())) {
                    flag = false; break;
                }
            }
            if (flag) {
                is_final[node_id] = true;
                int sum = 0;
                double total_prob = 0.0;
                for (auto& edge: node.edge_list) {
                    sum += _getTerminateSize(edge.semantics.get(), param_size_list) * edge.weight;
                    total_prob += edge.weight;
                }
                average_list[node_id] = 1.0 * sum / total_prob;
            }
        }
        int best_node = -1, best_edge = -1;
        double best_average = 1e18;
        for (int node_id = 0; node_id < g->node_list.size(); ++node_id) {
            if (is_final[node_id]) continue;
            auto& node = g->node_list[node_id];
            for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
                double average = 1.0;
                bool flag = true;
                auto& edge = node.edge_list[edge_id];
                if (_isTerminateSemantics(edge.semantics.get())) continue;
                for (auto sub_id: edge.v_list) {
                    if (!is_final[sub_id]) {
                        flag = false; break;
                    }
                    average += average_list[sub_id];
                }
                if (average < best_average && flag) {
                    best_average = average; best_node = node_id; best_edge = edge_id;
                }
            }
        }
        if (best_node == -1) return false;

        auto& node = g->node_list[best_node];
        auto edge = node.edge_list[best_edge];
        std::unordered_map<std::string, PSemantics> param_map;
        for (int i = 0; i < fg->param_list.size(); ++i) {
            auto feature = fg->param_list[i].second->toString();
            param_map[feature] = semantics::buildParamSemantics(i, fg->param_list[i].first);
        }
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
                    info_list.emplace_back(fg->param_list[ps->id].second, sub_edge.weight);
                }
            }
            approx_num *= int(info_list.size());
            storage.push_back(info_list);
        }
        if (approx_num > size_limit * 50) return false;
        auto info_list = _collectAllProgram(storage, edge);
        int now = 0;
        for (auto& info: info_list) {
            if (!validator(info.first.get())) continue;
            info_list[now++] = info;
            auto feature = info.first->toString();
            if (param_map.count(feature) == 0) {
                int id = fg->param_list.size();
                fg->param_list.emplace_back(node.symbol->type, info.first);
                param_map[feature] = semantics::buildParamSemantics(id, node.symbol->type);
                if (param_map.size() > size_limit) return false;
            }
        }
        info_list.resize(now);

        for (int i = best_edge + 1; i < node.edge_list.size(); ++i) {
            node.edge_list[i - 1] = node.edge_list[i];
        }
        node.edge_list.pop_back();
        for (auto& p_info: info_list) {
            auto feature = p_info.first->toString();
            // LOG(INFO) << "new program " << feature << std::endl;
            assert(param_map.count(feature));
            node.edge_list.emplace_back(best_node, (std::vector<int>){}, p_info.second, param_map[feature]);
        }
        if (node.edge_list.empty()) _deleteEmptyNode(fg->graph);
        LOG(INFO) << "merge " << best_node << " " << best_edge << " " << node.edge_list.size() << std::endl;
        return true;
    }
}

TopDownGraphMatchStructure * FlattenGrammar::getMatchStructure(int node_id, const PProgram& p) const {
    auto feature = p->toString();
    auto it = param_map.find(feature);
    if (it != param_map.end()) {
        int param_id = it->second;
        int edge_id = param_index_list[param_id][node_id];
        if (edge_id != -1) {
            return new TopDownGraphMatchStructure(edge_id, program::buildParam(param_id, param_list[param_id].first), {});
        }
    }
    auto& node = graph->node_list[node_id];
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

void FlattenGrammar::buildParamIndex() {
    param_map.clear(); param_index_list.clear();
    for (int i = 0; i < param_list.size(); ++i) {
        param_index_list.emplace_back(graph->node_list.size(), -1);
    }
    for (int node_id = 0; node_id < graph->node_list.size(); ++node_id) {
        auto& node = graph->node_list[node_id];
        for (int edge_id = 0; edge_id < node.edge_list.size(); ++edge_id) {
            auto& edge = node.edge_list[edge_id];
            auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
            if (ps) param_index_list[ps->id][node_id] = edge_id;
        }
    }
    for (int i = 0; i < param_list.size(); ++i) {
        param_map[param_list[i].second->toString()] = i;
    }
    is_indexed = true;
}

TopDownGraphMatchStructure * FlattenGrammar::getMatchStructure(const PProgram& p) {
    if (!is_indexed) buildParamIndex();
    return getMatchStructure(0, p);
}

FlattenGrammar * selector::getFlattenGrammar(TopDownContextGraph *g, int size_limit, const ProgramValidator& validator) {
    std::vector<std::pair<PType, PProgram>> param_list;
    TypeList type_list;
    for (auto& node: g->node_list) {
        for (auto& edge: node.edge_list) {
            auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
            if (ps) {
                while (type_list.size() <= ps->id) type_list.emplace_back();
                if (ps->oup_type) type_list[ps->id] = ps->oup_type;
            }
        }
    }
    for (int i = 0; i < type_list.size(); ++i) {
        assert(type_list[i]);
        param_list.emplace_back(type_list[i], program::buildParam(i, type_list[i]));
    }
    auto* fg = new FlattenGrammar(param_list, g);
    while (_oneStepFlatten(fg, size_limit, validator)) {fg->is_indexed = false;}
    fg->graph->normalizeProbability();
    return fg;
}