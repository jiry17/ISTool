//
// Created by pro on 2022/9/23.
//

#ifndef ISTOOL_INCRE_INFO_H
#define ISTOOL_INCRE_INFO_H

#include "istool/incre/language/incre_lookup.h"
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

    class ComponentSemantics: public ConstSemantics {
    public:
        ComponentSemantics(const Data& _data, const std::string& _name);
        virtual ~ComponentSemantics() = default;
    };

    enum class ComponentType {
        BOTH, AUX, COMB
    };

    typedef std::function<Term(const TermList&)> TermBuilder;

    class SynthesisComponent {
    public:
        ComponentType type;
        TypeList inp_types;
        PType oup_type;
        PSemantics semantics;
        TermBuilder builder;
        InputComponentInfo info;
        virtual Term buildTerm(const TermList& term_list);
        SynthesisComponent(ComponentType _type, const TypeList& _inp_types, const PType& _oup_type, const PSemantics& _semantics, const TermBuilder& builder,
                const InputComponentInfo& _info);
        virtual ~SynthesisComponent() = default;
    };

    class IncreInfo {
    public:
        IncreProgram program;
        Context* ctx;
        AlignTypeInfoList pass_infos;
        IncreExamplePool* example_pool;
        std::vector<SynthesisComponent*> component_list;
        IncreInfo(const IncreProgram& _program, Context* _ctx, const AlignTypeInfoList& infos, IncreExamplePool* pool, const std::vector<SynthesisComponent*>& component_list);
        ~IncreInfo();
    };
}

namespace incre {
    Ty unfoldTypeWithLabeledCompress(const Ty& type, TypeContext* ctx);
    void checkAllLabelBounded(ProgramData* program);
    // IncreProgram eliminateUnboundedCreate(const IncreProgram& program);
    IncreProgram labelCompress(const IncreProgram& program);
    AlignTypeInfoList collectAlignType(const IncreProgram& program);
    std::vector<SynthesisComponent*> collectComponentList(Context* ctx, Env* env, const std::unordered_map<std::string, InputComponentInfo>& compress_map);
    IncreInfo* buildIncreInfo(const IncreProgram& program, Env* env);
}


#endif //ISTOOL_INCRE_INFO_H
