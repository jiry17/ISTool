//
// Created by 赵雨薇 on 2023/5/19.
//

#ifndef ISTOOL_INCRE_TO_HASKELL_H
#define ISTOOL_INCRE_TO_HASKELL_H

#include "istool/basic/grammar.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/language/incre.h"

namespace incre {
    void tyToHaskell(const std::shared_ptr<TyData> &ty);
    void patternToHaskell(const std::shared_ptr<PatternData> &pattern);
    void termToHaskell(const std::shared_ptr<TermData> &term);
    void termParamToHaskell(const std::shared_ptr<TermData> &term, const bool &first_fix = true, const bool &first_param = true);
    void bindingToHaskell(const std::shared_ptr<BindingData> &binding);
    void bindingTyToHaskell(const std::shared_ptr<BindingData> &binding, const std::string &name);
    void commandToHaskell(const std::shared_ptr<CommandData> &command);
    void preOutput();
    void postOutput(const std::vector<std::pair<Term, Data>> &io_pairs);
    void grammarToHaskell(Grammar *grammar, int func_num,
        std::unordered_map<std::string, int>& name_to_expr_num, int& next_expr_num);
    void evalToHaskell(Grammar *grammar, int func_num, std::unordered_map<std::string, int>& name_to_expr_num);
    void programToHaskell(const std::shared_ptr<ProgramData> &prog, 
        const std::vector<std::pair<Term, Data>> &io_pairs, 
        incre::IncreInfo *info,
        const incre::IncreAutoLifterSolver *solver, const std::string &path);
}

#endif //ISTOOL_INCRE_TO_HASKELL_H
