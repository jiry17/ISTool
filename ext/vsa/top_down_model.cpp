//
// Created by pro on 2022/1/3.
//

#include "istool/ext/vsa/top_down_model.h"
#include "istool/sygus/parser/json_util.h"
#include "glog/logging.h"
#include <unordered_set>
#include <cmath>
#include <queue>

TopDownModel::TopDownModel(const PTopDownContext &_start, double _default_weight, ProbModelType _type):
    start(_start), default_weight(_default_weight), prob_type(_type) {
}

namespace {
    void _collectAllInfo(const PTopDownContext& context, const TopDownModel* model, Program* program, std::vector<std::pair<PTopDownContext, Semantics*>>& res) {
        res.emplace_back(context, program->semantics.get());
        for (int i = 0; i < program->sub_list.size(); ++i) {
            auto next_context = model->move(context.get(), program->semantics.get(), i);
            _collectAllInfo(next_context, model, program->sub_list[i].get(), res);
        }
    }

    std::vector<std::pair<PTopDownContext, Semantics*>> _collectAllInfo(Program* program, const TopDownModel* model) {
        std::vector<std::pair<PTopDownContext, Semantics*>> res;
        _collectAllInfo(model->start, model, program, res);
        return res;
    }
}

double TopDownModel::getWeight(Program* program, ProbModelType type) const {
    auto res = _collectAllInfo(program, this);
    assert(res.size() > 0);
    double weight = getWeight(res[0].first.get(), res[0].second, type);
    for (int i = 1; i < res.size(); ++i) {
        auto current = getWeight(res[i].first.get(), res[i].second, type);
        weight = ext::vsa::addProb(weight, current, type);
    }
    return weight;
}
double TopDownModel::getWeight(TopDownContext *ctx, Semantics *sem, ProbModelType oup_type) const {
    return ext::vsa::changeProbModel(getWeight(ctx, sem), prob_type, oup_type);
}

namespace {
    class NGramContext: public TopDownContext {
    public:
        std::vector<std::pair<std::string, int>> ctx;
        virtual std::string toString() const {
            std::string res;
            for (auto& w: ctx) {
                res += "@" + w.first + "@" + std::to_string(w.second);
            }
            return res;
        }
        NGramContext(const std::vector<std::pair<std::string, int>>& _ctx): ctx(_ctx) {
        }
    };

    PTopDownContext _buildInit(unsigned int depth) {
        std::vector<std::pair<std::string, int>> ctx(depth, {"", 0});
        return std::make_shared<NGramContext>(ctx);
    }
}

NGramModel::NGramModel(unsigned int _depth, const SemanticsAbstracter &_sa, double _inf): depth(_depth),
    sa(_sa), TopDownModel(_buildInit(_depth), _inf) {
}

double NGramModel::getWeight(TopDownContext *ctx, Semantics *sem) const {
    auto feature = ctx->toString() + "@" + sa(sem);
    if (weight_map.find(feature) == weight_map.end()) return default_weight;
    return weight_map.find(feature)->second;
}
void NGramModel::set(const std::vector<std::pair<std::string, double> > &weight_list) {
    weight_map.clear();
    for (const auto& item: weight_list) {
        weight_map[item.first] = item.second;
    }
}
PTopDownContext NGramModel::move(TopDownContext *ctx, Semantics *sem, int pos) const {
    auto* ng_ctx = dynamic_cast<NGramContext*>(ctx);
    if (!ng_ctx) {
        LOG(FATAL) << "NGramModel supports only NGramContexts";
    }
    std::vector<std::pair<std::string, int>> ctx_list;
    if (depth == 0) return std::make_shared<NGramContext>(ctx_list);
    for (int i = 1; i < ng_ctx->ctx.size(); ++i) {
        ctx_list.push_back(ng_ctx->ctx[i]);
    }
    ctx_list.emplace_back(sa(sem), pos);
    return std::make_shared<NGramContext>(ctx_list);
}

NGramModel * ext::vsa::getSizeModel() {
    SemanticsAbstracter sa = [](Semantics* sem){return "";};
    return new NGramModel(0, sa, 1);
}

void ext::vsa::learn(NGramModel *model, const ProgramList &program_list) {
    std::unordered_map<std::string, std::unordered_map<std::string, int>> info_map;
    for (const auto& program: program_list) {
        auto info_list = _collectAllInfo(program.get(), model);
        for (const auto& info: info_list) {
            std::string ctx_name = info.first->toString();
            ++info_map[ctx_name][model->sa(info.second)];
        }
    }
    std::vector<std::pair<std::string, double>> weight_list;
    for (const auto& item: info_map) {
        auto ctx_map = item.second;
        std::string ctx_name = item.first;
        int tot = 0;
        for (const auto& sem_info: ctx_map) tot += sem_info.second;
        for (const auto& sem_info: ctx_map) {
            auto sem_name = sem_info.first;
            double weight = -std::log(double(sem_info.second) / tot);
            weight_list.emplace_back(ctx_name + "@" + sem_name, weight);
        }
    }
    model->set(weight_list);
}

namespace {
    struct CEdge;

    struct CNode {
        VSANode* node;
        PTopDownContext ctx;
        std::vector<CEdge> edge_list;
        int id = 0;
        CNode(VSANode* _node, const PTopDownContext& _ctx): node(_node), ctx(_ctx) {
        }
    };

    struct CEdge {
        PSemantics semantics;
        std::vector<CNode*> node_list;
        double weight;
        CEdge(const PSemantics& _sem, const std::vector<CNode*>& _node_list, double _weight): semantics(_sem), node_list(_node_list), weight(_weight) {
        }
    };

    CNode* _buildCVSA(VSANode* node, const PTopDownContext& ctx, TopDownModel* model, TimeGuard* guard, std::unordered_map<std::string, CNode*>& cache) {
        TimeCheck(guard);
        std::string feature = node->toString() + "@" + ctx->toString();
        if (cache.find(feature) != cache.end()) return cache[feature];
        auto* now = new CNode(node, ctx); cache[feature] = now;
        for (const auto& edge: node->edge_list) {
            std::vector<CNode*> cnode_list;
            for (int i = 0; i < edge.node_list.size(); ++i) {
                auto next_ctx = model->move(ctx.get(), edge.semantics.get(), i);
                cnode_list.push_back(_buildCVSA(edge.node_list[i], next_ctx, model, guard, cache));
            }
            now->edge_list.emplace_back(edge.semantics, cnode_list, model->getWeight(ctx.get(), edge.semantics.get(), ProbModelType::NEG_LOG_PROB));
        }
        return now;
    }

    CNode* _buildCVSA(VSANode* node, TopDownModel* model, TimeGuard* guard) {
        std::unordered_map<std::string, CNode*> cache;
        return _buildCVSA(node, model->start, model, guard, cache);
    }

    void _collectAllNode(CNode* node, std::unordered_set<CNode*>& cache, std::vector<CNode*>& res) {
        if (cache.find(node) != cache.end()) return;
        cache.insert(node); res.push_back(node);
        for (const auto& edge: node->edge_list) {
            for (auto* sub_node: edge.node_list) {
                _collectAllNode(sub_node, cache, res);
            }
        }
    }

    std::vector<CNode*> _collectAllNode(CNode* root) {
        std::vector<CNode*> res; std::unordered_set<CNode*> cache;
        _collectAllNode(root, cache, res);
        for (int i = 0; i < res.size(); ++i) {
            res[i]->id = i;
        }
        return res;
    }

    void _deleteCVSA(CNode* root) {
        auto node_list = _collectAllNode(root);
        for (auto* node: node_list) delete node;
    }

    double _getWeightForAcyclic(CNode* node, std::vector<double>& min_weight, std::vector<bool>& is_visited) {
        if (is_visited[node->id]) return min_weight[node->id];
        double &res = min_weight[node->id]; is_visited[node->id] = true;
        for (const auto& edge: node->edge_list) {
            double edge_size = edge.weight;
            for (auto* sub_node: edge.node_list) {
                edge_size += _getWeightForAcyclic(sub_node, min_weight, is_visited);
            }
            res = std::min(res, edge_size);
        }
        return res;
    }

    void _getWeightForAcyclic(CNode* node, std::vector<double>& min_weight) {
        std::vector<bool> is_visited(min_weight.size(), false);
        _getWeightForAcyclic(node, min_weight, is_visited);
    }

    struct NodeWeightInfo {
        int id;
        double weight;
    };
    int operator > (const NodeWeightInfo& x, const NodeWeightInfo& y) {
        return x.weight > y.weight;
    }

    const double KWeightInf = 1e100;
    const double KEps = 1e-8;

    void _getWeightForCyclic(CNode* root, const std::vector<CNode*>& node_list, std::vector<double>& min_weight) {
        std::priority_queue<NodeWeightInfo, std::vector<NodeWeightInfo>, std::greater<>>Q;
        for (auto* node: node_list) {
            bool is_end = false;
            for (const auto& edge: node->edge_list) {
                if (edge.node_list.empty()) {
                    min_weight[node->id] = std::min(min_weight[node->id], edge.weight);
                    is_end = true;
                }
            }
            if (is_end) {
                Q.push({node->id, min_weight[node->id]});
            }
        }

        std::vector<std::vector<std::pair<int, int>>> rev_edge_list(node_list.size());
        for (const auto& node: node_list) {
            for (int i = 0; i < node->edge_list.size(); ++i) {
                for (const auto& sub_node: node->edge_list[i].node_list) {
                    rev_edge_list[sub_node->id].emplace_back(node->id, i);
                }
            }
        }

        while (!Q.empty()) {
            auto info = Q.top(); Q.pop();
            if (min_weight[info.id] < info.weight - KEps) continue;
            for (const auto& rev_edge: rev_edge_list[info.id]) {
                auto* pre_node = node_list[rev_edge.first];
                auto& pre_edge = pre_node->edge_list[rev_edge.second];
                double weight = pre_edge.weight;
                for (auto* sub_node: pre_edge.node_list) {
                    weight += min_weight[sub_node->id];
                }
                if (weight < min_weight[pre_node->id] - KEps) {
                    min_weight[pre_node->id] = weight;
                    Q.push({pre_node->id, weight});
                }
            }
        }
    }

    PProgram _constructProgram(CNode* node, const std::vector<double>& weight_list) {
        for (auto& edge: node->edge_list) {
            double weight = edge.weight;
            for (auto* sub_node: edge.node_list) {
                weight += weight_list[sub_node->id];
            }
            if (std::fabs(weight - weight_list[node->id]) < KEps) {
                ProgramList sub_list;
                for (auto* sub_node: edge.node_list) {
                    sub_list.push_back(_constructProgram(sub_node, weight_list));
                }
                return std::make_shared<Program>(edge.semantics, sub_list);
            }
        }
        LOG(FATAL) << "There is no matched program";
    }
}

PProgram ext::vsa::getBestProgram(VSANode* root, TopDownModel *model, TimeGuard *guard) {
    int n = indexVSANode(root);
    bool is_acyclic = isAcyclic(root, n);
    auto* c_root = _buildCVSA(root, model, guard);
    std::vector<CNode*> node_list = _collectAllNode(c_root);
    std::vector<double> min_weight_list(node_list.size(), KWeightInf);
    if (is_acyclic) {
        _getWeightForAcyclic(c_root, min_weight_list);
    } else {
        _getWeightForCyclic(c_root, node_list, min_weight_list);
    }
    auto res = _constructProgram(c_root, min_weight_list);
    _deleteCVSA(c_root);
    return res;
}

const SemanticsAbstracter ext::vsa::KDefaultAbstracter = [](Semantics* s) -> std::string {
    auto* cs = dynamic_cast<ConstSemantics*>(s);
    if (cs) return "Const";
    auto* ps = dynamic_cast<ParamSemantics*>(s);
    if (ps) return "Param";
    return s->getName();
};

NGramModel * ext::vsa::loadDefaultNGramModel(const std::string &model_file_path) {
    auto root = json::loadJsonFromFile(model_file_path);
    int depth = root["depth"].asInt();
    double default_value = root["default"].asDouble();
    auto* model = new NGramModel(depth, KDefaultAbstracter, default_value);
    std::vector<std::pair<std::string, double>> weight_list;
    for (const auto& sub_node: root["weight"]) {
        std::string name = sub_node["name"].asString();
        double weight = sub_node["weight"].asDouble();
        weight_list.emplace_back(name, weight);
    }
    model->set(weight_list);
    return model;
}

void ext::vsa::saveNGramModel(NGramModel *model, const std::string &model_file_path) {
    Json::Value root;
    root["depth"] = model->depth; root["default"] = model->default_weight;
    Json::Value weight_node;
    for (const auto& info: model->weight_map) {
        Json::Value info_node;
        info_node["name"] = info.first; info_node["weight"] = info.second;
        weight_node.append(info_node);
    }
    root["weight"] = weight_node;
    json::saveJsonToFile(root, model_file_path);
}

double ext::vsa::getTrueProb(double prob, ProbModelType type) {
    switch (type) {
        case ProbModelType::NORMAL_PROB: return prob;
        case ProbModelType::NEG_LOG_PROB: return std::exp(-prob);
    }
    LOG(FATAL) << "Unknown prob type";
}

double ext::vsa::getRepresentedProb(double prob, ProbModelType type) {
    switch (type) {
        case ProbModelType::NORMAL_PROB: return prob;
        case ProbModelType::NEG_LOG_PROB: return -std::log(prob);
    }
    LOG(FATAL) << "Unknown prob type";
}

double ext::vsa::changeProbModel(double prob, ProbModelType source_type, ProbModelType target_type) {
    if (source_type == target_type) return prob;
    auto true_prob = ext::vsa::getTrueProb(prob, source_type);
    return ext::vsa::getRepresentedProb(true_prob, target_type);
}

double ext::vsa::addProb(double prob_x, double prob_y, ProbModelType type) {
    switch (type) {
        case ProbModelType::NORMAL_PROB: return prob_x * prob_y;
        case ProbModelType::NEG_LOG_PROB: return prob_x + prob_y;
    }
    LOG(FATAL) << "Unknown prob type";
}