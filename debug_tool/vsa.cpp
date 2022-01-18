//
// Created by pro on 2022/1/2.
//

#include "debug_tool/vsa.h"
#include "istool/sygus/theory/witness/clia/clia_wit_value.h"
#include "istool/sygus/theory/basic/clia/clia_value.h"
#include "glog/logging.h"

PProgram debug::getRandomProgram(VSANode *root) {
    VSAEdgeList end_list, mid_list;
    for (const auto& edge: root->edge_list) {
        if (edge.node_list.empty()) {
            end_list.push_back(edge);
        } else mid_list.push_back(edge);
    }
    VSAEdge& edge = end_list.empty() ? mid_list[rand() % mid_list.size()] : end_list[rand() % end_list.size()];
    ProgramList sub_list;
    for (auto* nod: edge.node_list) {
        sub_list.push_back(getRandomProgram(nod));
    }
    return std::make_shared<Program>(edge.semantics, sub_list);
}

namespace {
    WitnessTerm _getWitnessList(VSANode* node) {
        auto* sn = dynamic_cast<SingleVSANode*>(node);
        if (sn) return {sn->oup};
        auto* mn = dynamic_cast<MultiExampleVSANode*>(node);
        assert(mn); WitnessTerm res;
        for (const auto& d: _getWitnessList(mn->l)) {
            res.push_back(d);
        }
        for (const auto& d: _getWitnessList(mn->r)) {
            res.push_back(d);
        }
        return res;
    }
}

void debug::testVSA(VSANode *root, const ExampleList &example_list, Env* env) {
    auto wl = _getWitnessList(root);
    for (int _ = 0; _ < 100; ++_) {
        auto p = getRandomProgram(root);
        for (int i = 0; i < wl.size(); ++i) {
            assert(wl[i]->isInclude(env->run(p.get(), example_list[i])));
        }
    }
}

void debug::testVSAEdge(VSANode *node, const VSAEdge& edge) {
    WitnessTerm x = _getWitnessList(node);
    auto* fs = dynamic_cast<FullExecutedSemantics*>(edge.semantics.get());
    if (!fs) return;
    auto* ps = dynamic_cast<ParamSemantics*>(edge.semantics.get());
    if (ps) return;
    WitnessList y;
    for (auto& sub_node: edge.node_list) y.push_back(_getWitnessList(sub_node));
    for (int i = 0; i < x.size(); ++i) {
        DataList inp_list; bool flag = false;
        for (const auto& d: y) {
            auto iv = dynamic_cast<DirectWitnessValue*>(d[i].get());
            if (iv) inp_list.push_back(iv->d); else flag = true;
        }
        if (flag) continue;
        if (!x[i]->isInclude(fs->run(std::move(inp_list), nullptr))) {
            LOG(INFO) << "Test error " << node->toString() << " " << edge.semantics->getName();
            for (auto& dl: y) {
                for (auto& d: dl) std::cout << " " << d->toString(); std::cout << std::endl;
            }
            assert(0);
        }
    }
}

bool debug::containProgram(VSANode *root, Program* program) {
    int n = program->sub_list.size();
    for (const auto& edge: root->edge_list) {
        if (edge.semantics->getName() == program->semantics->getName()) {
            bool is_contain = true;
            assert(program->sub_list.size() == edge.node_list.size());
            for (int i = 0; i < n; ++i) {
                if (!containProgram(edge.node_list[i], program->sub_list[i].get())) {
                    is_contain = false; break;
                }
            }
            if (is_contain) return true;
        }
    }
    return false;
}

void debug::viewVSA(VSANode *node) {
    std::cout << "node: " << node->toString() << std::endl;
    for (int i = 0; i < node->edge_list.size(); ++i) {
        std::cout << "#" << i << " " << node->edge_list[i].toString() << std::endl;
    }
    std::string s;
    std::cin >> s;
    if (s == "e") return;
    if (s == "v") {
        int k1, k2; std::cin >> k1 >> k2;
        viewVSA(node->edge_list[k1].node_list[k2]);
    }
}

namespace {

    DataList _getAllContent(const WitnessData& data) {
        auto* dv = dynamic_cast<DirectWitnessValue*>(data.get());
        if (dv) return {dv->d};
        auto* rv = dynamic_cast<RangeWitnessValue*>(data.get());
        assert(rv);
        DataList res;
        for (int i = rv->l; i <= rv->r; ++i) res.push_back(BuildData(Int, i));
        return res;
    }


    void _testWitness(Semantics *semantics, const WitnessData &oup, const WitnessTerm &res) {
        for (auto& inp: res) if (dynamic_cast<TotalWitnessValue*>(inp.get())) return;
        auto* fs = dynamic_cast<FullExecutedSemantics*>(semantics);
        if (!fs) return;
        if (dynamic_cast<ParamSemantics*>(semantics)) return;
        DataStorage inp_list;
        for (auto& inp: res) inp_list.push_back(_getAllContent(inp));
        inp_list = data::cartesianProduct(inp_list);
        for (const auto& inp: inp_list) {
            auto now = inp;
            auto current_oup = fs->run(std::move(now), nullptr);
            if (!oup->isInclude(current_oup)) {
                LOG(FATAL) << "Invalid " << semantics->getName() << " " << data::dataList2String(inp) << "->" << current_oup.toString() << " " << oup->toString() << std::endl;
            }
        }
    }

}

void debug::testWitness(Semantics *semantics, const WitnessData &oup, const WitnessList &res) {
    for (auto& term: res) {
        _testWitness(semantics, oup, term);
    }
}