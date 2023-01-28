//
// Created by pro on 2023/1/21.
//

#ifndef ISTOOL_INCRE_FUNC_TYPE_H
#define ISTOOL_INCRE_FUNC_TYPE_H

#include "istool/incre/language/incre.h"

namespace incre::autolabel {
    typedef std::pair<std::string, Ty> FuncParamInfo;
    class TmFunc: public TermData {
    public:
        Ty rec_res_type;
        std::string func_name;
        std::vector<FuncParamInfo> param_list;
        Term content;
        TmFunc(const std::string& _func_name, const Ty& _rec_res_type, const std::vector<FuncParamInfo>& _param_list, const Term& _content);
        virtual std::string toString() const;
        virtual ~TmFunc() = default;
    };

    Term buildFuncTerm(const std::string& name, const Term& def);
    Term unfoldFuncTerm(TmFunc* func_term);
    IncreProgram buildFuncTerm(ProgramData* program);
    IncreProgram unfoldFuncTerm(ProgramData* program);
    Ty getTypeWithFunc(const Term& term, TypeContext* ctx);
}

#endif //ISTOOL_INCRE_FUNC_TYPE_H
