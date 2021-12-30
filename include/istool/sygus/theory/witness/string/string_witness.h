//
// Created by pro on 2021/12/31.
//

#ifndef ISTOOL_STRING_WITNESS_H
#define ISTOOL_STRING_WITNESS_H

#include "istool/ext/vsa/witness.h"

#define DefineStringWitnessFunction(name) \
class name ## WitnessFunction: public WitnessFunction { \
public: \
    DataList* const_list; \
    name ## WitnessFunction(DataList* _const_list): const_list(_const_list) {} \
    virtual WitnessList witness(const WitnessData& oup); \
};

DefineWitnessFunction(StringCat)
DefineStringWitnessFunction(StringReplace)
DefineStringWitnessFunction(StringAt)

class IntToStrWitnessFunction: public WitnessFunction {
public:
    Data* inf;
    IntToStrWitnessFunction(Data* _inf): inf(_inf) {}
    virtual WitnessList witness(const WitnessData& oup);
};

class StringSubStrWitnessFunction: public WitnessFunction {
public:
    DataList* const_list;
    Data* int_min, *int_max;
    StringSubStrWitnessFunction(DataList* _const_list, Data* _int_min, Data* _int_max);
    virtual WitnessList witness(const WitnessData& oup);
};

DefineStringWitnessFunction(StringLen)
DefineStringWitnessFunction(StringIndexOf)
DefineStringWitnessFunction(StringPrefixOf)
DefineStringWitnessFunction(StringSuffixOf)
DefineStringWitnessFunction(StringContains)
DefineStringWitnessFunction(StrToInt)

namespace theory {
    namespace string {
        extern const std::string KStringConstList = "String@Consts";
    }
}

#endif //ISTOOL_STRING_WITNESS_H
