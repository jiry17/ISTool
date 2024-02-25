//
// Created by pro on 2022/10/8.
//

#ifndef ISTOOL_INCRE_PRINTER_H
#define ISTOOL_INCRE_PRINTER_H

#include "istool/incre/language/incre_program.h"
#include "istool/incre/language/incre_semantics.h"
#include "istool/incre/language/incre_syntax.h"

namespace incre::syntax {
    void printTy(const Ty &ty);
    void printPattern(const Pattern &pattern);
    void printTerm(const Term &term);
    void printBinding(const std::shared_ptr<Binding> &binding);
}

namespace incre {
    void printCommand(const Command &command);
    void printProgram(const IncreProgram &prog, const std::string &path = {}, bool is_align_mark = false);
}

#endif //ISTOOL_INCRE_PRINTER_H
