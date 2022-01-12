//
// Created by pro on 2022/1/3.
//

#ifndef ISTOOL_TOP_DOWN_MODEL_H
#define ISTOOL_TOP_DOWN_MODEL_H

#include "vsa.h"
#include "istool/basic/time_guard.h"

class TopDownContext {
public:
    virtual std::string toString() const = 0;
    virtual ~TopDownContext() = default;
};

typedef std::shared_ptr<TopDownContext> PTopDownContext;

class TopDownModel {
public:
    PTopDownContext start;
    double default_weight;
    TopDownModel(const PTopDownContext& start, double _inf);
    virtual PTopDownContext move(TopDownContext* ctx, Semantics* sem, int pos) const = 0;
    virtual double getWeight(TopDownContext* ctx, Semantics* sem) const = 0;
    double getWeight(Program* program) const;
    virtual ~TopDownModel() = default;
};

typedef std::function<std::string(Semantics*)> SemanticsAbstracter;

class NGramModel: public TopDownModel {
public:
    unsigned int depth;
    SemanticsAbstracter sa;
    std::unordered_map<std::string, double> weight_map;
    NGramModel(unsigned int _depth, const SemanticsAbstracter &_sa, double _default_weight = 1e90);
    void set(const std::vector<std::pair<std::string, double>>& weight_list);
    virtual PTopDownContext move(TopDownContext* ctx, Semantics* sem, int pos) const;
    virtual double getWeight(TopDownContext* ctx, Semantics* sem) const;
    virtual ~NGramModel() = default;
};

namespace ext {
    namespace vsa {
        NGramModel *getSizeModel();
        void learn(NGramModel *model, const ProgramList &program_list);
        PProgram getBestProgram(VSANode* root, TopDownModel* model, TimeGuard* guard = nullptr);
    }
}

#endif //ISTOOL_TOP_DOWN_MODEL_H
