//
// Created by pro on 2022/9/21.
//

#ifndef ISTOOL_INCRE_INSTRU_TYPE_H
#define ISTOOL_INCRE_INSTRU_TYPE_H

#include "istool/incre/language/incre.h"

namespace incre {
    class VLabeledCompress: public VCompress {
    public:
        int id;
        VLabeledCompress(const Data& _v, int _id);
        virtual ~VLabeledCompress() = default;
    };

    class TyLabeledCompress: public TyCompress {
    public:
        int id;
        TyLabeledCompress(const Ty& _ty, int _id);
        virtual std::string toString() const;
        virtual ~TyLabeledCompress() = default;
    };

    class TmLabeledCreate: public TmCreate {
    public:
        int id;
        TmLabeledCreate(const Term& _content, int _id);
        virtual ~TmLabeledCreate() = default;
    };

    class TmLabeledPass: public TmPass {
    public:
        int tau_id;
        std::unordered_map<std::string, Data> subst_info;
        TmLabeledPass(const std::vector<std::string> &names, const TermList &_defs, const Term &_content, int _tau_id,
                const std::unordered_map<std::string, Data>& _subst_info = {});
        void addSubst(const std::string& name, const Data& data);
        virtual ~TmLabeledPass() = default;
    };
}

#endif //ISTOOL_INCRE_INSTRU_TYPE_H
