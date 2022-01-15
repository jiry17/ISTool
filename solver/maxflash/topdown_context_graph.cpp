//
// Created by pro on 2022/1/13.
//

#include "istool/solver/maxflash/topdown_context_graph.h"
#include <queue>


TopDownContextGraph::Node::Node(NonTerminal *_symbol, const PTopDownContext& _context): symbol(_symbol), context(_context) {
}
std::string TopDownContextGraph::Node::toString() const {
    return symbol->name + "@" + context->toString();
}
TopDownContextGraph::Edge::Edge(int _u, const std::vector<int> &_v_list, double _weight, PSemantics &_semantics):
    u(_u), v_list(_v_list), weight(_weight), semantics(_semantics) {
}

namespace {
    double KDoubleINF = 1e100;
    double KDoubleEps = 1e-6;
}

TopDownContextGraph::TopDownContextGraph(Grammar *g, TopDownModel *model) {
    std::unordered_map<std::string, int> node_map;
    auto initialize_node = [&](NonTerminal* symbol, const PTopDownContext& ctx) -> int {
        Node node(symbol, ctx);
        auto feature = node.toString();
        if (node_map.find(feature) != node_map.end()) return node_map[feature];
        int res = int(node_list.size());
        node_map[feature] = res;
        node_list.push_back(node);
        return res;
    };

    initialize_node(g->start, model->start);
    for (int id = 0; id < node_list.size(); ++id) {
        auto* symbol = node_list[id].symbol;
        auto* ctx = node_list[id].context.get();
        for (const auto& rule: symbol->rule_list) {
            double weight = model->getWeight(ctx, rule->semantics.get());
            std::vector<int> v_list;
            for (int i = 0; i < rule->param_list.size(); ++i) {
                auto next_context = model->move(ctx, rule->semantics.get(), i);
                v_list.push_back(initialize_node(rule->param_list[i], next_context));
            }
            node_list[id].edge_list.emplace_back(id, v_list, weight, rule->semantics);
        }
    }

    for (int i = 0; i < node_list.size(); ++i) node_list[i].lower_bound = KDoubleINF;
    std::vector<std::vector<std::pair<int, int>>> reversed_edge_list(node_list.size());
    for (int u = 0; u < node_list.size(); ++u) {
        for (int e_id = 0; e_id < node_list[u].edge_list.size(); ++e_id) {
            for (const auto& id: node_list[u].edge_list[e_id].v_list) {
                reversed_edge_list[id].emplace_back(u,e_id);
            }
        }
    }
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<>> Q;
    auto update = [&](int id, double new_lower_bound) -> void {
        if (node_list[id].lower_bound <= new_lower_bound + KDoubleEps) return;
        node_list[id].lower_bound = new_lower_bound;
        Q.push({id, new_lower_bound});
    };
    for (int i = 0; i < node_list.size(); ++i) {
        double lower_bound = KDoubleINF;
        for (auto& edge: node_list[i].edge_list) {
            if (edge.v_list.empty()) lower_bound = std::min(lower_bound, edge.weight);
        }
        if (lower_bound < KDoubleINF) update(i, lower_bound);
    }
    auto update_edge = [&](int u, int e_id) -> void {
        auto& edge = node_list[u].edge_list[e_id];
        double sum = edge.weight;
        for (auto id: edge.v_list) {
            sum += node_list[id].lower_bound;
        }
        update(u, sum);
    };

    while (!Q.empty()) {
        int id = Q.top().second; double w = Q.top().first; Q.pop();
        if (std::abs(node_list[id].lower_bound - w) > KDoubleEps) continue;
        for (const auto& edge: reversed_edge_list[id]) {
            update_edge(edge.first, edge.second);
        }
    }
}

void TopDownContextGraph::print() const {
    for (int i = 0; i < node_list.size(); ++i) {
        std::cout << "node #" << i << " " << node_list[i].toString() << " >= " << node_list[i].lower_bound << std::endl;
        for (const auto& edge: node_list[i].edge_list) {
            std::cout << "  " << edge.weight << ": " << edge.semantics->getName() << "(";
            for (int j = 0; j < edge.v_list.size(); ++j) {
                if (j) std::cout << ",";
                std::cout << edge.v_list[j];
            }
            std::cout << ")" << std::endl;
        }
    }
}