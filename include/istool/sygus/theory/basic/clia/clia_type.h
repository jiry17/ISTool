//
// Created by pro on 2021/12/19.
//

#ifndef ISTOOL_CLIA_TYPE_H
#define ISTOOL_CLIA_TYPE_H

#include "istool/basic/data.h"

class TInt: public Type {
public:
    virtual std::string getName();
    virtual bool equal(Type* type);
};

namespace theory {
    namespace clia {
        PType getTInt();
        PType getTBool();
    }
}

#endif //ISTOOL_CLIA_TYPE_H
