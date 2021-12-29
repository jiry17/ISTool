//
// Created by pro on 2021/12/29.
//

#include <algorithm>
#include <queue>
#include "istool/solver/vsa/vsa_builder.h"

bool TrivialPruner::isPrune(VSANode *node) {return false;}
void TrivialPruner::clear() {}

SizeLimitPruner::SizeLimitPruner(int _size_limit): size_limit(_size_limit), remain(size_limit) {
}
void SizeLimitPruner::clear() {remain = size_limit;}
bool SizeLimitPruner::isPrune(VSANode *node) {
    if (remain) {
        --remain; return false;
    }
    return true;
}

VSABuilder::VSABuilder(Grammar *_g, Pruner *_pruner): g(_g), pruner(_pruner) {
}
VSABuilder::~VSABuilder() {
    delete pruner;
}

PVSANode DFSVSABuilder::buildVSA(NonTerminal *nt, const WitnessData &oup, const DataList &inp_list, TimeGuard *guard, std::unordered_map<std::string, PVSANode> &cache) {
    std::string feature = nt->name + "@" + oup->toString();
    if (cache.find(feature) != cache.end()) return cache[feature];
    PVSANode node = std::make_shared<SingleVSANode>(nt, oup);
    cache[feature] = node;
    if (pruner->isPrune(node.get())) return node;
    TimeCheck(guard);
    for (auto* rule: nt->rule_list) {
        auto witness = ext->getWitness(rule->semantics.get(), oup, inp_list);
        for (auto& wl: witness) {
#ifdef DEBUG
            assert(wl.size() == rule->param_list.size());
#endif
            VSANodeList node_list;
            for (int i = 0; i < wl.size(); ++i) {
                auto sub_node = buildVSA(rule->param_list[i], wl[i], inp_list, guard, cache);
                node_list.push_back(sub_node);
            }
            node->edge_list.push_back(std::make_shared<VSAEdge>(rule->semantics, node_list));
        }
    }
    return node;
}

PVSANode DFSVSABuilder::buildVSA(const Data &oup, const DataList &inp_list, TimeGuard *guard) {
    pruner->clear();
    std::unordered_map<std::string, PVSANode> cache;
    auto wit_oup = std::make_shared<DirectWitnessValue>(oup);
    auto root = buildVSA(g->start, wit_oup, inp_list, guard, cache);
    ext::vsa::cleanUpVSA(root);
    return root;
}

namespace {
    struct EdgeMatchStatus {
        int ll, lr, rl, rr;
    };

    std::vector<EdgeMatchStatus> matchVSAEdgeList(const VSAEdgeList& l_list, const VSAEdgeList& r_list) {
        std::vector<EdgeMatchStatus> res;
        int l_pos = 0, r_pos = 0;
        while (l_pos < l_list.size() && r_pos < r_list.size()) {
            auto l_name = l_list[l_pos]->semantics->getName();
            auto r_name = r_list[r_pos]->semantics->getName();
            if (l_name != r_name) {
                if (l_name < r_name) ++l_pos; else ++r_pos;
                continue;
            }
            int l_ne = l_pos + 1;
            while (l_ne < l_list.size() && l_list[l_ne]->semantics->getName() == l_name) ++l_ne;
            int r_ne = r_pos + 1;
            while (r_ne < r_list.size() && r_list[r_ne]->semantics->getName() == r_name) ++r_ne;
            res.push_back({l_pos, l_ne, r_pos, r_ne});
            l_pos = l_ne; r_pos = r_ne;
        }
        return res;
    }
}

PVSANode DFSVSABuilder::mergeVSA(const PVSANode &l, const PVSANode &r, TimeGuard *guard, std::unordered_map<std::string, PVSANode> &cache) {
    std::string feature = std::to_string(l->id) + "@" + std::to_string(r->id);
    if (cache.find(feature) != cache.end()) return cache[feature];
    PVSANode node = std::make_shared<MultiExampleVSANode>(l, r);
    cache[feature] = node;
    if (pruner->isPrune(node.get())) return node;
    for (const auto& status: matchVSAEdgeList(l->edge_list, r->edge_list)) {
        for (int i = status.ll; i < status.lr; ++i)
            for (int j = status.rl; j < status.rr; ++j) {
                auto& le = l->edge_list[i], &re = r->edge_list[j];
#ifdef DEBUG
                assert(le->semantics->getName() == re->semantics->getName() && le->node_list.size() == re->node_list.size());
#endif
                VSANodeList node_list;
                for (int k = 0; k < le->node_list.size(); ++k) {
                    node_list.push_back(mergeVSA(le->node_list[k], re->node_list[k], guard, cache));
                }
                node->edge_list.push_back(std::make_shared<VSAEdge>(le->semantics, node_list));
            }
    }
    return node;
}

namespace {
    void orderEdgeList(const PVSANode& node, std::vector<bool>& cache) {
        if (cache[node->id]) return;
        cache[node->id] = true;
        std::sort(node->edge_list.begin(), node->edge_list.end(),
                [](const PVSAEdge& x, const PVSAEdge& y) {return x->semantics->getName() < y->semantics->getName();});
        for (const auto& e: node->edge_list) {
            for (auto& sub_node: e->node_list) {
                orderEdgeList(sub_node, cache);
            }
        }
    }

    void orderEdgeList(const PVSANode& node) {
        int n = ext::vsa::indexVSANode(node.get());
        std::vector<bool> cache(n);
        for (int i = 0; i < n; ++i) cache[i] = false;
        orderEdgeList(node, cache);
    }
}

PVSANode DFSVSABuilder::mergeVSA(const PVSANode &l, const PVSANode &r, TimeGuard *guard) {
    pruner->clear();
    orderEdgeList(l); orderEdgeList(r);
    std::unordered_map<std::string, PVSANode> cache;
    auto root = mergeVSA(l, r, guard, cache);
    ext::vsa::cleanUpVSA(root);
    return root;
}

PVSANode BFSBSABuilder::buildVSA(const Data &oup, const DataList &inp_list, TimeGuard *guard) {
    std::unordered_map<std::string, PVSANode> cache;
    std::queue<PVSANode> Q; pruner->clear();
    auto insert = [&](NonTerminal* nt, const WitnessData& d) {
        std::string feature = nt->name + "@" + d->toString();
        if (cache.find(feature) != cache.end()) return cache[feature];
        auto node = std::make_shared<SingleVSANode>(nt, d);
        if (!pruner->isPrune(node.get())) Q.push(node);
        return cache[feature] = node;
    };

    auto init_d = std::make_shared<DirectWitnessValue>(oup);
    auto root = insert(g->start, init_d);

    while (!Q.empty()) {
        TimeCheck(guard);
        auto node = Q.front(); Q.pop();
        auto* sn = dynamic_cast<SingleVSANode*>(node.get());
        for (auto* rule: node->symbol->rule_list) {
            auto witness = ext->getWitness(rule->semantics.get(), sn->oup, inp_list);
            for (auto& wl: witness) {
#ifdef DEBUG
                assert(wl.size() == rule->param_list.size());
#endif
                VSANodeList node_list;
                for (int i = 0; i < wl.size(); ++i) {
                    node_list.push_back(insert(rule->param_list[i], wl[i]));
                }
                node->edge_list.push_back(std::make_shared<VSAEdge>(rule->semantics, node_list));
            }
        }
    }

    ext::vsa::cleanUpVSA(root);
    return root;
}

PVSANode BFSBSABuilder::mergeVSA(const PVSANode &l, const PVSANode &r, TimeGuard *guard) {
    pruner->clear();
    orderEdgeList(l); orderEdgeList(r);
    std::unordered_map<std::string, PVSANode> cache;
    std::queue<PVSANode> Q;

    auto insert = [&](const PVSANode& l, const PVSANode& r) {
        std::string feature = std::to_string(l->id) + "@" + std::to_string(r->id);
        if (cache.find(feature) != cache.end()) return cache[feature];
        auto node = std::make_shared<MultiExampleVSANode>(l, r);
        if (!pruner->isPrune(node.get())) Q.push(node);
        return cache[feature] = node;
    };

    auto root = insert(l, r);
    while (!Q.empty()) {
        TimeCheck(guard);
        auto node = Q.front(); Q.pop();
        auto* mn = dynamic_cast<MultiExampleVSANode*>(node.get());
        auto l_node = mn->l, r_node = mn->r;
        for (auto status: matchVSAEdgeList(l_node->edge_list, r_node->edge_list)) {
            for (int i = status.ll; i < status.lr; ++i)
                for (int j = status.rl; j < status.rr; ++j) {
                    auto& le = l_node->edge_list[i], &re = r_node->edge_list[j];
#ifdef DEBUG
                    assert(le->semantics->getName() == re->semantics->getName() && le->node_list.size() == re->node_list.size());
#endif
                    VSANodeList node_list;
                    for (int k = 0; k < le->node_list.size(); ++k) {
                        node_list.push_back(insert(le->node_list[k], re->node_list[k]));
                    }
                    node->edge_list.push_back(std::make_shared<VSAEdge>(le->semantics, node_list));
                }
        }
    }

    ext::vsa::cleanUpVSA(root);
    return root;
}