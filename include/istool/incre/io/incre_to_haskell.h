//
// Created by zyw on 2023/5/19.
//

#ifndef ISTOOL_INCRE_TO_HASKELL_H
#define ISTOOL_INCRE_TO_HASKELL_H

#include "istool/basic/grammar.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/language/incre.h"

namespace incre {
    void tyToHaskell(const std::shared_ptr<TyData> &ty, bool &in_def_ind = false);
    void patternToHaskell(const std::shared_ptr<PatternData> &pattern);
    void termToHaskell(const std::shared_ptr<TermData> &term);
    void termParamToHaskell(const std::shared_ptr<TermData> &term, const bool &first_fix = true, const bool &first_param = true);
    void bindingToHaskell(const std::shared_ptr<BindingData> &binding);
    void bindingTyToHaskell(const std::shared_ptr<BindingData> &binding, const std::string &name);
    void commandToHaskell(const std::shared_ptr<CommandData> &command);
    void preOutput();
    void outputRefEnv();
    void postOutput(const std::vector<std::pair<Term, Data>> &io_pairs);
    // get env
    void envToHaskell(std::vector<std::pair<std::vector<std::string>, Grammar *> > &final_grammar,
        std::vector<std::pair<PType, int> > &env_type_list);
    // get grammar for each hole
    void grammarToHaskell(Grammar *grammar, int func_num,
        std::unordered_map<std::string, int>& name_to_expr_num, int& next_expr_num);
    // get program space for each hole
    void spaceToHaskell(Grammar *grammar, int func_num,
        std::unordered_map<std::string, int>& name_to_expr_num, int& next_expr_num);
    // get eval function for each hole
    void evalToHaskell(Grammar *grammar, int func_num, std::unordered_map<std::string, int>& name_to_expr_num,
        std::vector<std::string>& param_list, std::vector<std::pair<PType, int> > &env_type_list);
    void getEvalString(std::vector<std::string> &eval_string_for_each_hole,
        std::vector<std::pair<std::vector<std::string>, Grammar *> > final_grammar,
        TyList &final_type_list,
        std::vector<std::pair<PType, int> > &env_type_list);
    void programToHaskell(const std::shared_ptr<ProgramData> &prog, 
        const std::vector<std::pair<Term, Data>> &io_pairs, 
        incre::IncreInfo *info,
        const incre::IncreAutoLifterSolver *solver, const std::string &path,
        TyList &final_type_list);
}

#endif //ISTOOL_INCRE_TO_HASKELL_H
