//
// Created by pro on 2023/1/22.
//

#ifndef ISTOOL_INCRE_AUTOLABEL_GREEDY_SOLVER_H
#define ISTOOL_INCRE_AUTOLABEL_GREEDY_SOLVER_H

#include "incre_autolabel.h"

namespace incre::autolabel {
    class TmLabeledLet: public TmLet {
    public:
        Ty var_type;
        TmLabeledLet(const std::string& name, const Ty& _var_type, const Term& def, const Term& content);
        virtual std::string toString() const;
        virtual ~TmLabeledLet() = default;
    };

    class GreedyAutoLabelSolver : public AutoLabelSolver {
        IncreProgram prepareLetType();
        IncreProgram prepareVarLabel(ProgramData* program);
        IncreProgram assignLabel();
        IncreProgram assignAlign(ProgramData* program);
    public:
        GreedyAutoLabelSolver(const AutoLabelTask& task);
        virtual IncreProgram label();
        virtual ~GreedyAutoLabelSolver() = default;
    };
}

#endif //ISTOOL_INCRE_AUTOLABEL_GREEDY_SOLVER_H
