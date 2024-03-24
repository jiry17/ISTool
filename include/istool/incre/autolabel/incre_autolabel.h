//
// Created by pro on 2023/1/22.
//

#ifndef ISTOOL_INCRE_AUTOLABEL_H
#define ISTOOL_INCRE_AUTOLABEL_H

#include "istool/incre/language/incre_context.h"
#include "istool/incre/language/incre_program.h"
#include "istool/incre/language/incre_rewriter.h"
#include "istool/incre/language/incre_semantics.h"
#include "istool/incre/language/incre_syntax.h"
#include "istool/incre/language/incre_types.h"

namespace incre::autolabel {
    class AutoLabelSolver {
    public:
        IncreProgram init_program;
        AutoLabelSolver(const IncreProgram& init_program);
        virtual IncreProgram label() = 0;
        virtual ~AutoLabelSolver() = default;
    };
}

#endif //ISTOOL_INCRE_AUTOLABEL_H
