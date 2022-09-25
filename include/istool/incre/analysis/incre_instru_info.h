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
        TmLabeledPass* term;
        std::vector<std::pair<std::string, Ty>> inp_types;
        Ty oup_type;
        PassTypeInfoData(TmLabeledPass* term, const std::unordered_map<std::string, Ty>& type_ctx, const Ty& _oup_type);
        void print() const;
        ~PassTypeInfoData() = default;
    };
    typedef std::shared_ptr<PassTypeInfoData> PassTypeInfo;
    typedef std::vector<PassTypeInfo> PassTypeInfoList;

    struct FComponent {
        std::string name;
        Ty type;
        Data d;
        FComponent(const std::string& _name, const Ty& _type, const Data& _d);
        ~FComponent() = default;
    };

    class IncreInfo {
    public:
        IncreProgram program;
        Context* ctx;
        PassTypeInfoList pass_infos;
        IncreExamplePool* example_pool;
        std::vector<FComponent> f_components;
        IncreInfo(const IncreProgram& _program, Context* _ctx, const PassTypeInfoList& infos, IncreExamplePool* pool);
        ~IncreInfo();
    };
}

namespace incre {
    Ty unfoldAll(const Ty& type, TypeContext* ctx);
    IncreProgram eliminateUnboundedCreate(const IncreProgram& program);
    IncreProgram labelCompress(const IncreProgram& program);
    PassTypeInfoList collectPassType(const IncreProgram& program);
    IncreInfo* buildIncreInfo(const IncreProgram& program);
}


#endif //ISTOOL_INCRE_INFO_H
