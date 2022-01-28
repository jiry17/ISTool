//
// Created by pro on 2022/1/27.
//

#include "istool/selector/samplesy/vsa_seed_generator.h"
#include "glog/logging.h"

namespace {
    void _collectAllNode(VSANode* node, std::vector<VSANode*>& node_list) {
        if (node_list[node->id]) return;
        node_list[node->id] = node;
        for (const auto& edge: node->edge_list) {
            for (auto* sub_node: edge.node_list) {
                _collectAllNode(sub_node, node_list);
            }
        }
    }

    std::vector<double> _mul(const std::vector<double>& x, const std::vector<double>& y) {
        int n = x.size(), m = y.size();
        std::vector<double> res(n + m, 0.0);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                res[i + j + 1] += x[i] * y[j];
            }
        }
        while (!res.empty() && res[int(res.size()) - 1] < 1) res.pop_back();
        return res;
    }

    std::vector<double> _sum(const std::vector<double>& x, const std::vector<double>& y) {
        int n = x.size(), m = y.size();
        std::vector<double> res(std::max(n, m), 0.0);
        for (int i = 0; i < std::max(n, m); ++i) {
            res[i] = (i < x.size() ? x[i] : 0.0) + (i < y.size() ? y[i] : 0.0);
        }
        while (!res.empty() && res[int(res.size()) - 1] < 1) res.pop_back();
        return res;
    }

    int _getSample(const std::vector<double>& A, Env* env) {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        double w = d(env->random_engine);
        double sum = 0.0;
        for (int i = 0; i < A.size(); ++i) sum += A[i];
        assert(sum >= 1.0);
        for (int i = 0; i < A.size(); ++i) {
            double k = A[i] / sum;
            if (k + 1e-8 >= w) return i;
            w -= k;
        }
        assert(0);
    }

    void _mergeIntStorage(int pos, const std::vector<std::vector<int>>& A, std::vector<int>& tmp, std::vector<std::vector<int>>& res) {
        if (pos == A.size()) {
            res.push_back(tmp); return;
        }
        for (int w: A[pos]) {
            tmp[pos] = w;
            _mergeIntStorage(pos, A, tmp, res);
        }
    }

    std::vector<std::vector<int>> _mergeIntStorage(const std::vector<std::vector<int>>& A) {
        std::vector<std::vector<int>> result;
        std::vector<int> tmp(A.size());
        _mergeIntStorage(0, A, tmp, result);
        return result;
    }
}

VSASizeBasedSampler::VSASizeBasedSampler(Env *_env): env(_env) {
}
std::vector<double> VSASizeBasedSampler::getEdgeSize(const VSAEdge &edge) {
    std::vector<double> res(2, 0.0); res[1] = 1.0;
    for (auto* node: edge.node_list) {
        res = _mul(res, size_list[node->id]);
    }
    return res;
}
void VSASizeBasedSampler::setRoot(VSANode *new_root) {
    root = new_root; int n = ext::vsa::indexVSANode(root);
    node_list.resize(n);
    for (int i = 0; i < node_list.size(); ++i) node_list[i] = nullptr;
    for (int i = n - 1; i >= 0; ++i) {
        size_list[i].clear();
        for (const auto& edge: node_list[i]->edge_list) {
            size_list[i] = _sum(size_list[i], getEdgeSize(edge));
        }
    }
}
PProgram VSASizeBasedSampler::sampleProgram(const VSAEdge &edge, int target_size) {
    std::vector<std::vector<int>> size_pool;
    for (auto* node: edge.node_list) {
        std::vector<int> possible_size;
        for (int i = 0; i < target_size; ++i) {
            if (size_list[node->id][i] >= 1.0) possible_size.push_back(i);
        }
        size_pool.push_back(possible_size);
    }
    std::vector<std::vector<int>> possible_size_plan;
    std::vector<double> weight_list;
    for (auto& size_plan: _mergeIntStorage(size_pool)) {
        int total_size = 1;
        for (int size: size_plan) total_size += size;
        if (total_size != target_size) continue;
        double weight = 1.0;
        for (int i = 0; i < edge.node_list.size(); ++i) {
            weight *= size_list[edge.node_list[i]->id][size_plan[i]];
        }
        weight_list.push_back(weight);
    }

    int plan_id = _getSample(weight_list, env);
    ProgramList sub_list;
    for (int i = 0; i < edge.node_list.size(); ++i) {
        sub_list.push_back(sampleProgram(edge.node_list[i], possible_size_plan[plan_id][i]));
    }
    return std::make_shared<Program>(edge.semantics, sub_list);
}
PProgram VSASizeBasedSampler::sampleProgram(VSANode *node, int target_size) {
    assert(target_size < node_list.size());
    std::vector<double> edge_size_list;
    for (const auto& edge: node->edge_list) {
        auto edge_size = getEdgeSize(edge);
        if (target_size >= edge_size.size()) edge_size_list.push_back(0.0);
        else edge_size_list.push_back(edge_size[target_size]);
    }
    int edge_id = _getSample(edge_size_list, env);
    return sampleProgram(node->edge_list[edge_id], target_size);
}
PProgram VSASizeBasedSampler::sampleNext() {
    int target_size = _getSample(size_list[0], env);
    return sampleProgram(root, target_size);
}

VSASeedGenerator::VSASeedGenerator(const PVSABuilder& _builder, VSASampler* _sampler): builder(_builder), sampler(_sampler) {
    root = builder->buildFullVSA();
    if (!ext::vsa::isAcyclic(root)) {
        LOG(FATAL) << "VSASampleSy requires the grammar to be acyclic";
    }
}

void VSASeedGenerator::addExample(const IOExample &example) {
    auto next_root = builder->buildVSA(example.second, example.first, nullptr);
    root = builder->mergeVSA(root, next_root, nullptr);
}

ProgramList VSASeedGenerator::getSeeds(int num, double time_limit) {
    auto* guard = new TimeGuard(time_limit);
    sampler->setRoot(root);
    ProgramList res;
    while (guard->getRemainTime() >= 0. && res.size() < num) {
        res.push_back(sampler->sampleNext());
    }
    return res;
}