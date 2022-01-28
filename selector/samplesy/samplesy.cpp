//
// Created by pro on 2022/1/27.
//

#include "istool/selector/samplesy/samplesy.h"
#include "istool/sygus/theory/basic/clia/clia.h"

namespace {
    int KDefaultTimeOut = 60;
    int KDefaultSampleNum = 1000;
}

const std::string selector::samplesy::KSampleTimeOutLimit = "SampleSy@TimeOut";
const std::string selector::samplesy::KSampleNumLimit = "SampleSy@SampleNum";

SampleSy::SampleSy(Specification* _spec, Splitor *_splitor, SeedGenerator* _gen, EquivalenceChecker* _checker):
    CompleteSelector(_spec), splitor(_splitor), gen(_gen), checker(_checker) {
    auto* env = splitor->example_space->env;
    KSampleTimeOut = env->getConstRef(selector::samplesy::KSampleTimeOutLimit, BuildData(Int, KDefaultTimeOut * 1000));
    KSampleNum = env->getConstRef(selector::samplesy::KSampleNumLimit, BuildData(Int, KDefaultSampleNum));
}
PProgram SampleSy::getNextAction(Example *example) {
    auto res = checker->isAllEquivalent();
    if (!res.second) {
        return gen->getSeeds(1, 1e9)[0];
    }
    double time_limit = theory::clia::getIntValue(*KSampleTimeOut) / 1000.;
    auto seed_list = gen->getSeeds(theory::clia::getIntValue(*KSampleNum), time_limit);
    seed_list.push_back(res.first); seed_list.push_back(res.second);
    splitor->getSplitExample(nullptr, seed_list, example);
    return nullptr;
}
void SampleSy::addExample(const IOExample &example) {
    gen->addExample(example);
    checker->addExample(example);
}
SampleSy::~SampleSy() {
    delete splitor; delete gen; delete checker;
}