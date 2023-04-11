//
// Created by pro on 2022/9/23.
//

#ifndef ISTOOL_INCRE_INFO_H
#define ISTOOL_INCRE_INFO_H

#include "istool/incre/language/incre_lookup.h"
#include "istool/incre/grammar/incre_component_collector.h"
#include "incre_instru_runtime.h"

namespace incre {

    class AlignTypeInfoData {
    public:
        TmLabeledAlign* term;
        Term _term;
        std::vector<std::pair<std::string, Ty>> inp_types;
        Ty oup_type;
        int command_id;
        AlignTypeInfoData(const Term& __term, const std::unordered_map<std::string, Ty>& type_ctx, const Ty& _oup_type, int _command_id);
        void print() const;
        int getId() const;
        ~AlignTypeInfoData() = default;
    };
    typedef std::shared_ptr<AlignTypeInfoData> AlignTypeInfo;
    typedef std::vector<AlignTypeInfo> AlignTypeInfoList;


    class IncreInfo {
    public:
        IncreProgram program;
        Context* ctx;
        AlignTypeInfoList align_infos;
        IncreExamplePool* example_pool;
        grammar::ComponentPool component_pool;
        IncreInfo(const IncreProgram& _program, Context* _ctx, const AlignTypeInfoList& infos, IncreExamplePool* pool, const grammar::ComponentPool& _pool);
        ~IncreInfo();
    };
}

namespace incre {
    Ty unfoldTypeWithLabeledCompress(const Ty& type, TypeContext* ctx);
    void checkAllLabelBounded(ProgramData* program);
    // IncreProgram eliminateUnboundedCreate(const IncreProgram& program);
    IncreProgram labelCompress(const IncreProgram& program);
    AlignTypeInfoList collectAlignType(const IncreProgram& program);
    void prepareEnv(Env* env);
    IncreInfo* buildIncreInfo(const IncreProgram& program, Env* env);
}


#endif //ISTOOL_INCRE_INFO_H
