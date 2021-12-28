//
// Created by pro on 2021/12/28.
//

#ifndef ISTOOL_STRING_TYPE_H
#define ISTOOL_STRING_TYPE_H

#include "istool/basic/type.h"


class TString: public Type {
public:
    virtual std::string getName();
    virtual bool equal(Type* type);
};

namespace theory {
    namespace string {
        PType getTString();
    }
}


#endif //ISTOOL_STRING_TYPE_H
