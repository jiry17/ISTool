//
// Created by pro on 2023/1/22.
//

#ifndef ISTOOL_INCRE_AUTOLABEL_H
#define ISTOOL_INCRE_AUTOLABEL_H

#include "istool/incre/language/incre.h"

namespace incre::autolabel {
    class AutoLabelTask {
    public:
        IncreProgram program;
        std::unordered_map<std::string, Ty> target_type_map;
        Ty getTargetType(const std::string& name) const;
        void updateFuncType();
        AutoLabelTask(const IncreProgram& _program, const std::unordered_map<std::string, Ty>& _target_type_map);
    };

    class AutoLabelSolver {
    public:
        AutoLabelTask task;
        AutoLabelSolver(const AutoLabelTask& task);
        virtual IncreProgram label() = 0;
        virtual ~AutoLabelSolver() = default;
    };

    AutoLabelTask initTask(ProgramData* program);
}

#endif //ISTOOL_INCRE_AUTOLABEL_H
