//
// Created by pro on 2022/1/27.
//

#ifndef ISTOOL_VSA_SEED_GENERATOR_H
#define ISTOOL_VSA_SEED_GENERATOR_H

#include "samplesy.h"
#include "different_program_generator.h"
#include "istool/solver/vsa/vsa_builder.h"
#include <stack>
#include <unordered_set>

class VSASampler {
public:
    virtual void setRoot(VSANode* new_root) = 0;
    virtual PProgram sampleNext() = 0;
    ~VSASampler() = default;
};

class VSASizeBasedSampler: public VSASampler {
    std::vector<std::vector<std::vector<double>>> edge_size_pool;
    std::vector<double> getEdgeSize(const VSAEdge& edge);
    void calculateNodeSize(VSANode* node, std::vector<bool>& visited);
    PProgram sampleProgram(VSANode* node, int target_size);
    PProgram sampleProgram(const VSAEdge& edge, int target_size);
public:
    VSANode* root = nullptr;
    Env* env;
    std::vector<VSANode*> node_list;
    std::vector<std::vector<double>> size_list;
    VSASizeBasedSampler(Env* _env);
    virtual void setRoot(VSANode* new_root);
    virtual PProgram sampleNext();
    ~VSASizeBasedSampler() = default;
};

class VSASeedGenerator: public SeedGenerator {
public:
    PVSABuilder builder;
    VSANode* root;
    VSASampler* sampler;
    VSASeedGenerator(const PVSABuilder& _builder, VSASampler* _sampler);
    virtual void addExample(const IOExample& example);
    virtual ProgramList getSeeds(int num, double time_limit);
    ~VSASeedGenerator() = default;
};

class FiniteVSASeedGenerator: public SeedGenerator {
public:
    IOExampleList io_examples;
    PVSABuilder builder;
    VSANode* root;
    VSASampler* sampler;
    DifferentProgramGenerator* g;
    FiniteVSASeedGenerator(const PVSABuilder& _builder, VSASampler* _sampler, DifferentProgramGenerator* _g, FiniteIOExampleSpace* io_space);
    virtual void addExample(const IOExample& example);
    virtual ProgramList getSeeds(int num, double time_limit);
    virtual ~FiniteVSASeedGenerator() = default;
};

#endif //ISTOOL_VSA_SEED_GENERATOR_H
