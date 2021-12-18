//
// Created by pro on 2021/12/5.
//

#ifndef ISTOOL_VERIFIER_H
#define ISTOOL_VERIFIER_H

#include "program.h"
#include "example_space.h"

class Verifier {
public:
    virtual PExample verify(const PProgram& program) const = 0;
    virtual ~Verifier() = default;
};

class FiniteExampleVerifier: public Verifier {
public:
    std::shared_ptr<FiniteExampleSpace> example_space;
    FiniteExampleVerifier(const std::shared_ptr<FiniteExampleSpace> &_example_space);
    virtual PExample verify(const PProgram& program) const;
    virtual ~FiniteExampleVerifier() = default;
};

class OccamVerifier: public Verifier {
public:
    int factor;
    std::shared_ptr<StreamedExampleSpace> example_space;
    OccamVerifier(const std::shared_ptr<StreamedExampleSpace>& example_space, int _factor = 100);
    virtual PExample verify(const PProgram& program) const;
    virtual ~OccamVerifier() = default;
};

#endif //ISTOOL_VERIFIER_H
