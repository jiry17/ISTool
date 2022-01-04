//
// Created by pro on 2022/1/2.
//

#include "debug_tool/vsa.h"
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

void debug::testVSA(VSANode *root, const ExampleList &example_list) {
    auto wl = _getWitnessList(root);
    for (int _ = 0; _ < 100; ++_) {
        auto p = getRandomProgram(root);
        for (int i = 0; i < wl.size(); ++i) {
            assert(wl[i]->isInclude(program::run(p.get(), example_list[i])));
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
