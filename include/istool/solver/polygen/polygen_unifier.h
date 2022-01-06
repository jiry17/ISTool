//
// Created by pro on 2022/1/7.
//

#ifndef ISTOOL_POLYGEN_UNIFIER_H
#define ISTOOL_POLYGEN_UNIFIER_H

#include "istool/solver/stun/stun.h"

class PolyGenUnifier: public Unifier {
    SolverBuilder builder;
    bool KIsUseTerm;
    IOExampleSpace* io_space;
    PProgram synthesizeCondition(const PSynthInfo& info, const IOExampleList& positive_list, const IOExampleList& negative_list, TimeGuard* guard);
public:
    PolyGenUnifier(Specification* spec, const PSynthInfo& info, const SolverBuilder& _builder);
    virtual PProgram unify(const ProgramList& term_list, const ExampleList& example_list, TimeGuard* guard = nullptr) = 0;
    virtual ~PolyGenUnifier();
};

namespace solver {
    namespace polygen {
        extern const std::string KIsUseTermName;
    }
}

#endif //ISTOOL_POLYGEN_UNIFIER_H
