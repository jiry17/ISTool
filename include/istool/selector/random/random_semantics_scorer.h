//
// Created by pro on 2022/4/27.
//

#ifndef ISTOOL_RANDOM_SEMANTICS_SCORER_H
#define ISTOOL_RANDOM_SEMANTICS_SCORER_H

#include "istool/selector/selector.h"
#include "istool/selector/random/grammar_flatter.h"
#include "istool/solver/maxflash/topdown_context_graph.h"
#include "istool/ext/vsa/top_down_model.h"

typedef long double RandomSemanticsScore;

namespace selector::random {
    class TerminateEdgeInfo {
    public:
        double weight;
        TerminateEdgeInfo(double _weight);
        virtual Data getOutput(const DataList& inp) const = 0;
        virtual ~TerminateEdgeInfo() = default;
    };

    class ConstTerminateEdgeInfo: public TerminateEdgeInfo {
    public:
        Data w;
        ConstTerminateEdgeInfo(const Data& _w, double _weight);
        virtual Data getOutput(const DataList& inp) const;
        virtual ~ConstTerminateEdgeInfo() = default;
    };

    class ParamTerminateEdgeInfo: public TerminateEdgeInfo {
    public:
        int param_id;
        ParamTerminateEdgeInfo(int _param_id, double _weight);
        virtual Data getOutput(const DataList& inp) const;
        virtual ~ParamTerminateEdgeInfo() = default;
    };

    class PairMatchInfoCache {
        void initRes();
    public:
        std::vector<TerminateEdgeInfo*> edge_list;
        std::vector<std::vector<int>> match_info;
        RandomSemanticsScore * res;
        int example_num;
        PairMatchInfoCache(const TopDownContextGraph::Node& node);
        int getEdgeIdForSemantics(Semantics* semantics);
        void pushExample(const DataList& inp);
        void popExample();
        void getTmpPairStateWeight(const DataList& inp, RandomSemanticsScore* tmp);
        void getOneSideMatchWeight(Semantics* sem, RandomSemanticsScore* tmp);
        ~PairMatchInfoCache();
    };

    class TripleMatchInfoCache {
        static std::vector<int> K2T5[5];
        static std::vector<int> K5Size;
        static void prepare2T5(int example_num);
        int get5State(int fp, int gp, int fg) const;
    public:
        std::vector<TerminateEdgeInfo*> edge_list;
        std::vector<std::vector<int>> match_info;
        int example_num;
        int p_id;
        RandomSemanticsScore* res;
        TripleMatchInfoCache(PairMatchInfoCache* two_cache, Semantics* s);
        void getTmpTripleStateWeight(const DataList& inp, RandomSemanticsScore* tmp);
        ~TripleMatchInfoCache();
    };

    class CachePool {
    public:
        TopDownContextGraph* graph;
        std::vector<PairMatchInfoCache*> pair_cache_list;
        std::unordered_map<std::string, TripleMatchInfoCache*> triple_cache_map;
        CachePool(TopDownContextGraph* _graph);
        void pushExample(const Example& inp);
        void getTripleMatchRes(int node_id, Semantics* s, const Example& inp, RandomSemanticsScore* res);
        void getPairMatchRes(int node_id, const Example& inp, RandomSemanticsScore* res);
        void getOneSizePairMatchRes(int node_id, Semantics* s, RandomSemanticsScore* res);
        void popExample();
        ~CachePool();
    };
}

class RandomSemanticsScorer {
public:
    RandomSemanticsScore getTripleScore(const PProgram& p, const DataStorage& inp_list);
    RandomSemanticsScore getPairScore(const DataStorage& inp_list);
    DataStorage getFlattenInpStorage(const DataStorage& inp_list);
    double KOutputSize;
    FlattenGrammar* fg;
    Env* env;
    RandomSemanticsScorer(Env* _env, FlattenGrammar* _fg, double _KOutputSize);
    ~RandomSemanticsScorer();
};

class CachedRandomSemanticsScorer {
public:
    RandomSemanticsScore getTripleScore(const PProgram& p, const Example& example);
    RandomSemanticsScore getPairScore(const Example& example);
    void pushExample(const Example& inp);
    void popExample();
    selector::random::CachePool* cache;
    RandomSemanticsScore KOutputSize;
    int example_num;
    FlattenGrammar* fg;
    Env* env;
    CachedRandomSemanticsScorer(Env* env, FlattenGrammar* _fg, RandomSemanticsScore _KOutputSize);
    ~CachedRandomSemanticsScorer();
};
#endif //ISTOOL_RANDOM_SEMANTICS_SCORER_H
