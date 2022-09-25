//
// Created by pro on 2022/9/25.
//

#ifndef ISTOOL_TANS_H
#define ISTOOL_TANS_H

#include "istool/incre/language/incre.h"

namespace incre {

    class TIncreInductive: public SimpleType {
    public:
        Ty _type;
        TyInductive* type;
        virtual ~TIncreInductive() = default;
        virtual std::string getName();
        virtual PType clone(const TypeList &params);
        TIncreInductive(const Ty& _type);
    };

    PType typeFromIncre(const Ty& type);
    Ty typeToIncre(Type* type);
    Term termToIncre(Program* program, const TermList& inps);
}

#endif //ISTOOL_TANS_H
