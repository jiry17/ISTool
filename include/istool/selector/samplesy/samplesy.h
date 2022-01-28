//
// Created by pro on 2022/1/27.
//

#ifndef ISTOOL_SAMPLESY_H
#define ISTOOL_SAMPLESY_H

#include "istool/selector/selector.h"
#include "istool/selector/split/splitor.h"

class SeedGenerator {
public:
    virtual void addExample(const IOExample& example) = 0;
    virtual ProgramList getSeeds(int num, double time_limit) = 0;
    virtual ~SeedGenerator() = default;
};

class EquivalenceChecker {
public:
    virtual void addExample(const IOExample& example) = 0;
    virtual std::pair<PProgram, PProgram> isAllEquivalent() = 0;
    virtual ~EquivalenceChecker() = default;
};

class SampleSy: public CompleteSelector {
public:
    Splitor* splitor;
    SeedGenerator* gen;
    EquivalenceChecker* checker;
    Data* KSampleTimeOut, *KSampleNum;
    SampleSy(Specification* _spec, Splitor* _splitor, SeedGenerator* _gen, EquivalenceChecker* checker);
    virtual PProgram getNextAction(Example* example);
    virtual void addExample(const IOExample& example);
    virtual ~SampleSy();
};

namespace selector::samplesy {
    extern const std::string KSampleTimeOutLimit;
    extern const std::string KSampleNumLimit;
}

#endif //ISTOOL_SAMPLESY_H
