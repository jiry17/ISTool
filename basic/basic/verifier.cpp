//
// Created by pro on 2021/12/5.
//

#include "verifier.h"
#include "glog/logging.h"

FiniteExampleVerifier::FiniteExampleVerifier(const std::shared_ptr<FiniteExampleSpace> &_example_space):
    example_space(_example_space) {
}
PExample FiniteExampleVerifier::verify(const PProgram &program) const {
    for (int i = 0; i < example_space->size(); ++i) {
        auto example = example_space->get(i);
        if (!example_space->get(i)->isSatisfy(program)) {
            return example;
        }
    }
    return nullptr;
}

OccamVerifier::OccamVerifier(const std::shared_ptr<StreamedExampleSpace> &_example_space, int _factor):
    example_space(_example_space), factor(_factor) {
}
PExample OccamVerifier::verify(const PProgram &program) const {
    int example_num = program->size() * factor;
    for (int i = 0; i < example_num; ++i) {
        auto example = example_space->get(i);
        if (!example->isSatisfy(program)) return example;
    }
    return nullptr;
}