//
// Created by pro on 2022/9/23.
//

#ifndef ISTOOL_INCRE_INFO_H
#define ISTOOL_INCRE_INFO_H

#include "istool/incre/language/incre.h"
#include "incre_instru_runtime.h"

namespace incre {

    class PassTypeInfoData {
    public:
        std::unordered_map<std::string, Ty> inp_types;
        Ty oup_type;
        PassTypeInfoData(const std::unordered_map<std::string, Ty>& _inp_types, const Ty& _oup_type);
        ~PassTypeInfoData() = default;
    };
    typedef std::shared_ptr<PassTypeInfoData> PassTypeInfo;
    typedef std::vector<PassTypeInfo> PassTypeInfoList;

    class IncreInfo {
    public:
        IncreProgram program;
        PassTypeInfoList pass_infos;
    };
}

namespace incre {
    IncreProgram eliminateUnboundedCreate(const IncreProgram& program);
    IncreProgram labelCompress(const IncreProgram& program);
    PassTypeInfoList collectPassType(const IncreProgram& program);
}


#endif //ISTOOL_INCRE_INFO_H
