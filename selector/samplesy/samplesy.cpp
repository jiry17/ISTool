//
// Created by pro on 2022/1/27.
//

#include "istool/selector/samplesy/samplesy.h"
#include "istool/sygus/theory/basic/clia/clia.h"

namespace {
    int KDefaultTimeOut = 60;
    int KDefaultSampleNum = 5000;
}

const std::string selector::samplesy::KSampleTimeOutLimit = "SampleSy@TimeOut";
const std::string selector::samplesy::KSampleNumLimit = "SampleSy@SampleNum";

SampleSy::SampleSy(Specification* _spec, Splitor *_splitor, SeedGenerator* _gen, EquivalenceChecker* _checker):
    CompleteSelector(_spec, _checker), splitor(_splitor), gen(_gen) {
    auto* env = splitor->example_space->env;
    KSampleTimeOut = env->getConstRef(selector::samplesy::KSampleTimeOutLimit, BuildData(Int, KDefaultTimeOut * 1000));
    KSampleNum = env->getConstRef(selector::samplesy::KSampleNumLimit, BuildData(Int, KDefaultSampleNum));
}
Example SampleSy::getNextExample(const PProgram &x, const PProgram &y) {
    double time_limit = theory::clia::getIntValue(*KSampleTimeOut) / 1000.;
    auto seed_list = gen->getSeeds(theory::clia::getIntValue(*KSampleNum), time_limit);
    Example example;
    splitor->getDistinguishExample(x.get(), y.get(), seed_list, &example);
    return example;
}
void SampleSy::addExample(const IOExample &example) {
    gen->addExample(example);
}
SampleSy::~SampleSy() {
    delete splitor; delete gen;
}