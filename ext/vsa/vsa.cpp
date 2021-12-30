//
// Created by pro on 2021/12/29.
//

#include "istool/ext/vsa/vsa.h"
#include "glog/logging.h"
#include <unordered_set>
#include <queue>

VSAEdge::VSAEdge(const PSemantics &_semantics, const VSANodeList &_node_list): semantics(_semantics), node_list(_node_list) {
}
VSANode::VSANode(NonTerminal *_symbol, int _example_num): symbol(_symbol), example_num(_example_num), id(0) {
}


SingleVSANode::SingleVSANode(NonTerminal *_symbol, const WitnessData &_oup): VSANode(_symbol, 1), oup(_oup) {
}
MultiExampleVSANode::MultiExampleVSANode(const PVSANode &_l, const PVSANode &_r):
    l(_l), r(_r), VSANode(_l->symbol, _l->example_num + _r->example_num) {
    if (l->symbol->name != r->symbol->name) {
        LOG(INFO) << "Two subnodes of a MultiExampleVSANode should come from the same symbol";
    }
}

namespace {
    void _indexVSANode(VSANode* node, int &n, std::unordered_set<VSANode*>& node_set) {
        if (node_set.find(node) == node_set.end()) return;
        node_set.insert(node); node->id = n++;
        for (const auto& edge: node->edge_list) {
            for (const auto& sub_node: edge->node_list) {
                _indexVSANode(sub_node.get(), n, node_set);
            }
        }
    }
}

int ext::vsa::indexVSANode(VSANode* root) {
    int n = 0;
    std::unordered_set<VSANode*> node_set;
    _indexVSANode(root, n, node_set);
    return n;
}

namespace {
    void _collectAllNodes(const PVSANode& node, VSANodeList& node_list) {
        if (node_list[node->id]) return;
        node_list[node->id] = node;
        for (const auto& edge: node->edge_list) {
            for (const auto& sub_node: edge->node_list) {
                _collectAllNodes(sub_node, node_list);
            }
        }
    }
}

void ext::vsa::cleanUpVSA(const PVSANode& root) {
    int n = indexVSANode(root.get());
    VSANodeList node_list(n);
    _collectAllNodes(root, node_list);
    std::vector<std::vector<int>> edge_index(n);
    std::vector<std::vector<std::pair<int, int>>> reversed_edge(n);
    std::vector<bool> is_empty;
    for (const auto& node: node_list) {
        int node_id = node->id;
        for (int edge_id = 0; edge_id < node->edge_list.size(); ++edge_id) {
            auto* edge = node->edge_list[edge_id].get();
            edge_index[node_id].push_back(edge->node_list.size());
            for (const auto& sub_node: edge->node_list) {
                int sub_id = sub_node->id;
                reversed_edge[sub_id].emplace_back(node_id, edge_id);
            }
        }
    }
    std::queue<int> Q;
    auto insert = [&](int id) {
        if (!is_empty[id]) return;
        is_empty[id] = false; Q.push(id);
    };
    for (const auto& node: node_list) {
        for (auto index: edge_index[node->id]) {
            if (!index) {
                insert(node->id); break;
            }
        }
    }
    while (!Q.empty()) {
        int id = Q.front(); Q.pop();
        for (auto& re: reversed_edge[id]) {
            auto pre_node_id = re.first, pre_edge_id = re.second;
            edge_index[pre_node_id][pre_edge_id]--;
            if (!edge_index[pre_node_id][pre_edge_id]) insert(pre_node_id);
        }
    }
    for (auto& node: node_list) {
        int now = 0;
        for (auto& edge: node->edge_list) {
            bool is_empty_edge = false;
            for (auto &sub_node: edge->node_list) {
                if (is_empty[sub_node->id]) {
                    is_empty_edge = true;
                    break;
                }
            }
            if (!is_empty_edge) node->edge_list[now++] = edge;
        }
        node->edge_list.resize(now);
    }
}

namespace {
    bool _isAcyclic(VSANode* node, std::vector<bool>& in_stack, std::vector<bool>& visited) {
        if (visited[node->id]) return !in_stack[node->id];
        visited[node->id] = true; in_stack[node->id] = true;
        for (const auto& edge: node->edge_list) {
            for (const auto& sub_node: edge->node_list) {
                if (!_isAcyclic(sub_node.get(), in_stack, visited)) {
                    return false;
                }
            }
        }
        in_stack[node->id] = false;
        return true;
    }
}

bool ext::vsa::isAcyclic(VSANode *root, int n) {
    if (n == -1) n = indexVSANode(root);
    std::vector<bool> in_stack(n);
    std::vector<bool> visited(n);
    for (int i = 0; i < n; ++i) in_stack[i] = false, visited[i] = false;
    return _isAcyclic(root, in_stack, visited);
}

namespace {
    const int KSizeInf = 1e9;
    int _getSizeForAcyclicVSA(VSANode* node, std::vector<int>& min_size) {
        if (min_size[node->id] != KSizeInf) return min_size[node->id];
        int size = KSizeInf;
        for (const auto& edge: node->edge_list) {
            int edge_size = 1;
            for (const auto& sub_node: edge->node_list) {
                edge_size = std::min(KSizeInf, edge_size + _getSizeForAcyclicVSA(sub_node.get(), min_size));
            }
            size = std::min(size, edge_size);
        }
        return min_size[node->id] = size;
    }

    struct NodeSizeInfo {
        int id, size;
    };
    int operator > (const NodeSizeInfo& x, const NodeSizeInfo& y) {
        return x.size > y.size;
    }

    int _getSizeForCyclicVSA(const PVSANode& root, int n, std::vector<int>& min_size) {
        VSANodeList node_list(n); _collectAllNodes(root, node_list);
        std::vector<bool> in_queue(n);
        std::priority_queue<NodeSizeInfo, std::vector<NodeSizeInfo>, std::greater<>>Q;
        for (const auto& node: node_list) {
            for (const auto& edge: node->edge_list) {
                if (edge->node_list.empty()) {
                    min_size[node->id] = 1;
                    Q.push({node->id, 1});
                }
            }
        }

        std::vector<std::vector<std::pair<int, int>>> rev_edge_list(n);
        for (const auto& node: node_list) {
            for (int i = 0; i < node->edge_list.size(); ++i) {
                for (const auto& sub_node: node->edge_list[i]->node_list) {
                    rev_edge_list[sub_node->id].emplace_back(node->id, i);
                }
            }
        }

        while (!Q.empty()) {
            auto info = Q.top(); Q.pop();
            if (min_size[info.id] != info.size) continue;
            for (const auto& rev_edge: rev_edge_list[info.id]) {
                auto* pre_node = node_list[rev_edge.first].get();
                auto* pre_edge = pre_node->edge_list[rev_edge.second].get();
                int edge_size = 1;
                for (const auto& sub_node: pre_edge->node_list) {
                    edge_size = std::min(KSizeInf, edge_size + min_size[sub_node->id]);
                }
                if (edge_size < min_size[pre_node->id]) {
                    min_size[pre_node->id] = edge_size;
                    Q.push({pre_node->id, edge_size});
                }
            }
        }
    }

    PProgram constructMinProgram(VSANode* node, std::vector<int>& min_size, ProgramList& cache) {
        if (cache[node->id]) return cache[node->id];
        for (const auto& edge: node->edge_list) {
            int edge_size = 1;
            for (const auto& sub_node: edge->node_list) {
                edge_size = std::min(KSizeInf, edge_size + min_size[sub_node->id]);
            }
            if (edge_size == min_size[node->id]) {
                ProgramList sub_list;
                for (const auto& sub_node: edge->node_list) {
                    sub_list.push_back(constructMinProgram(sub_node.get(), min_size, cache));
                }
                return cache[node->id] = std::make_shared<Program>(edge->semantics, sub_list);
            }
        }
        assert(0);
    }

}

PProgram ext::vsa::getMinimalProgram(const PVSANode& root) {
    int n = indexVSANode(root.get());

    // get min size for all nodes
    std::vector<int> min_size(n);
    for (int i = 0; i < n; ++i) min_size[i] = KSizeInf;
    if (isAcyclic(root.get(), n)) {
        _getSizeForAcyclicVSA(root.get(), min_size);
    } else {
        _getSizeForCyclicVSA(root, n, min_size);
    }

    if (min_size[0] == KSizeInf) {
        LOG(FATAL) << "The given VSA is empty";
    }
    ProgramList cache(n);
    return constructMinProgram(root.get(), min_size, cache);

}