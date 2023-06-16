//
// Created by 赵雨薇 on 2023/5/19.
//

#include "istool/incre/io/incre_to_haskell.h"
#include "istool/basic/config.h"
#include "istool/basic/grammar.h"
#include "istool/incre/language/incre.h"
#include "glog/logging.h"
#include <fstream>
#include <iostream>
#include <set>
#include <cstring>

using namespace incre;
bool debug_haskell = false;
int floor_num_haskell = 0;
// save all constructors, the first letter of constructor should be upper character
std::set<std::string> construct;
// some func names need to be modified because are reserved words. 
// modification method: add "'" after name
std::set<std::string> modified_func_name = {"main", "sum"};
// pre_func_name is the actual func name, post_func_name is used in fix term
std::string pre_func_name, post_func_name;
bool in_fix_func = false;

namespace {
    bool _isNeedBracket(TermType def_type) {
        return !(def_type == incre::TermType::VALUE ||
                 def_type == incre::TermType::VAR || def_type == incre::TermType::TUPLE
                 || def_type == incre::TermType::PROJ);
    }

    void printSpace(int num) {
        while (num--) {
            std::cout << "    ";
        }
    }

    // output deriving clause for defined type
    void outputDeriving(std::string func_name) {
      printSpace(1);
      std::cout << "deriving stock (Generic, Show)" << std::endl;
      printSpace(1);
      std::cout << "deriving (Mergeable, EvaluateSym, ToCon " << func_name << ", ExtractSymbolics)" << std::endl;
      printSpace(2);
      std::cout << "via (Default " << func_name << ")" << std::endl;
    }
}

void incre::tyToHaskell(const std::shared_ptr<TyData> &ty) {
    if (debug_haskell) std::cout << std::endl << "[zyw: tyToHaskell] ";
    if (ty->getType() == TyType::INT) {
        if (debug_haskell) std::cout << std::endl;
        std::cout << "SymInteger";
    } else if (ty->getType() == TyType::VAR) {
        if (debug_haskell) std::cout << "[VAR]" << std::endl;
        auto* ty_var = dynamic_cast<TyVar*>(ty.get());
        std::cout << ty_var->name;
    } else if (ty->getType() == TyType::UNIT) {
        if (debug_haskell) std::cout << std::endl;
        std::cout << "Unit";
    } else if (ty->getType() == TyType::BOOL) {
        if (debug_haskell) std::cout << std::endl;
        std::cout << "SymBool";
    } else if (ty->getType() == TyType::TUPLE) {
        if (debug_haskell) std::cout << "[TUPLE]" << std::endl;
        auto* ty_tuple = dynamic_cast<TyTuple*>(ty.get());
        bool flag = false;
        for (auto& tuple_field : ty_tuple->fields) {
            if (flag) {
                std::cout << " ";
                tyToHaskell(tuple_field);
            } else {
                tyToHaskell(tuple_field);
                flag = true;
            }
        }
    } else if (ty->getType() == TyType::IND) {
        if (debug_haskell) std::cout << "[IND]" << std::endl;
        auto* ty_ind = dynamic_cast<TyInductive*>(ty.get());
        std::cout << ty_ind->name << std::endl;
        bool flag = false;
        for (auto& ind_cons : ty_ind->constructors) {
            printSpace(floor_num_haskell);
            if (flag) {
                std::cout << "| ";
            } else {
                std::cout << "= ";
                flag = true;
            }
            construct.insert(ind_cons.first);
            // first char of constructors should be capitalised
            ind_cons.first[0] = std::toupper(ind_cons.first[0]);
            std::cout << ind_cons.first << " ";
            tyToHaskell(ind_cons.second);
            std::cout << std::endl;
        }
    } else if (ty->getType() == TyType::COMPRESS) {
        if (debug_haskell) std::cout << "[COMPRESS]" << std::endl;
        auto* ty_compress = dynamic_cast<TyCompress*>(ty.get());
        std::cout << "Compress ";
        tyToHaskell(ty_compress->content);
    } else if (ty->getType() == TyType::ARROW) {
        if (debug_haskell) std::cout << "[ARROW]" << std::endl;
        auto* ty_arrow = dynamic_cast<TyArrow*>(ty.get());
        incre::TyType source_type = ty_arrow->source->getType();
        bool need_bracket = !(source_type == incre::TyType::INT ||
                              source_type == incre::TyType::UNIT || source_type == incre::TyType::BOOL
                              || source_type == incre::TyType::VAR ||  source_type == incre::TyType::COMPRESS);
        if (need_bracket) std::cout << "(";
        tyToHaskell(ty_arrow->source);
        if (need_bracket) std::cout << ")";
        std::cout << " -> ";
        tyToHaskell(ty_arrow->target);
    } else {
        LOG(FATAL) << "Unknown Ty";
    }
}

void incre::patternToHaskell(const std::shared_ptr<PatternData> &pattern) {
    if (debug_haskell) std::cout << std::endl << "[zyw: patternToHaskell]" << std::endl;
    if (pattern->getType() == PatternType::UNDER_SCORE) {
        if (debug_haskell) std::cout << std::endl;
        std::cout << "_";
    } else if (pattern->getType() == PatternType::VAR) {
        if (debug_haskell) std::cout << "[VAR]" << std::endl;
        auto* pt_var = dynamic_cast<PtVar*>(pattern.get());
        std::cout << pt_var->name;
    } else if (pattern->getType() == PatternType::TUPLE) {
        if (debug_haskell) std::cout << "[TUPLE]" << std::endl;
        auto* pt_tuple = dynamic_cast<PtTuple*>(pattern.get());
        bool flag = false;
        for (auto& pat : pt_tuple->pattern_list) {
            if (flag) {
                std::cout << " ";
            } else {
                flag = true;
            }
            bool need_bracket = (pat->getType() == PatternType::CONSTRUCTOR
                    || pat->getType() == PatternType::TUPLE);
            if (need_bracket) std::cout << "(";
            patternToHaskell(pat);
            if (need_bracket) std::cout << ")";
        }
    } else if (pattern->getType() == PatternType::CONSTRUCTOR) {
        if (debug_haskell) std::cout << "[CONSTRUCTOR]" << std::endl;
        auto* pt_cons = dynamic_cast<PtConstructor*>(pattern.get());
        // first char of constructors should be capitalised
        pt_cons->name[0] = std::toupper(pt_cons->name[0]);
        std::cout << pt_cons->name << " ";
        bool need_bracket = (pt_cons->pattern->getType() == PatternType::CONSTRUCTOR);
        if (need_bracket) std::cout << "(";
        patternToHaskell(pt_cons->pattern);
        if (need_bracket) std::cout << ")";
    } else {
        LOG(FATAL) << "Unknown pattern";
    }
}

void incre::termToHaskell(const std::shared_ptr<TermData> &term) {
    if (debug_haskell) std::cout << std::endl << "[zyw: termToHaskell]" << std::endl;
    if (term->getType() == TermType::VALUE) {
        if (debug_haskell) std::cout << "[VALUE]" << std::endl;
        auto* tm_value = dynamic_cast<TmValue*>(term.get());
        if (tm_value->data.isNull() || tm_value->data.toString() == "unit") {
            std::cout << "Unit";
        } else {
            
            std::cout << tm_value->data.toString();
        }
    } else if (term->getType() == TermType::IF) {
        if (debug_haskell) std::cout << "[IF]" << std::endl;
        auto* tm_if = dynamic_cast<TmIf*>(term.get());
        std::cout << "if (";
        termToHaskell(tm_if->c);
        std::cout << ") ";
        floor_num_haskell++;
        printSpace(floor_num_haskell);
        std::cout << "then ";
        termToHaskell(tm_if->t);
        std::cout << std::endl;
        printSpace(floor_num_haskell);
        std::cout << "else ";
        termToHaskell(tm_if->f);
        std::cout << std::endl;
        floor_num_haskell--;
    } else if (term->getType() == TermType::VAR) {
        if (debug_haskell) std::cout << "[VAR]" << std::endl;
        auto* tm_var = dynamic_cast<TmVar*>(term.get());
        if (construct.find(tm_var->name) != construct.end()) {
          tm_var->name[0] = std::toupper(tm_var->name[0]);
        }
        std::cout << tm_var->name;
        if (debug_haskell) std::cout << std::endl << "[Term_VAR_END]" << std::endl;
    } else if (term->getType() == TermType::LET) {
        if (debug_haskell) std::cout << "[LET]" << std::endl;
        auto* tm_let = dynamic_cast<TmLet*>(term.get());
        incre::TermType def_type = tm_let->def->getType();
        bool need_bracket = !(def_type == incre::TermType::VALUE ||
                              def_type == incre::TermType::VAR || def_type == incre::TermType::TUPLE
                              || def_type == incre::TermType::PROJ);
        std::cout << "let " << tm_let->name << " = ";
        if (need_bracket) std::cout << "(";
        termToHaskell(tm_let->def);
        if (need_bracket) std::cout << ")";
        std::cout << " in " << std::endl;
        floor_num_haskell++; printSpace(floor_num_haskell);
        termToHaskell(tm_let->content);
        floor_num_haskell--;
    } else if (term->getType() == TermType::TUPLE) {
        if (debug_haskell) std::cout << "[TUPLE]" << std::endl;
        auto* tm_tuple = dynamic_cast<TmTuple*>(term.get());
        bool flag = false;
        for (auto& tuple_field : tm_tuple->fields) {
            if (flag) {
                std::cout << " ";
            } else {
                flag = true;
            }
            incre::TermType content_type = tuple_field->getType();
            bool need_bracket = !(content_type == incre::TermType::VALUE ||
                                  content_type == incre::TermType::VAR || content_type == incre::TermType::TUPLE
                                  || content_type == incre::TermType::PROJ);
            if (need_bracket) std::cout << "(";
            termToHaskell(tuple_field);
            if (need_bracket) std::cout << ")";
        }
    } else if (term->getType() == TermType::PROJ) {
        if (debug_haskell) std::cout << "[PROJ]" << std::endl;
        auto* tm_proj = dynamic_cast<TmProj*>(term.get());
        incre::TermType content_type = tm_proj->content->getType();
        bool need_bracket = !(content_type == incre::TermType::VALUE ||
                              content_type == incre::TermType::VAR || content_type == incre::TermType::TUPLE
                              || content_type == incre::TermType::PROJ);
        if (need_bracket) std::cout << "(";
        termToHaskell(tm_proj->content);
        if (need_bracket) std::cout << ")";
        std::cout << "." << tm_proj->id;
    } else if (term->getType() == TermType::ABS) {
        if (debug_haskell) std::cout << "[ABS]" << std::endl;
        auto* tm_abs = dynamic_cast<TmAbs*>(term.get());
        termToHaskell(tm_abs->content);
    } else if (term->getType() == TermType::APP) {
        if (debug_haskell) std::cout << "[APP]" << std::endl;
        auto* tm_app = dynamic_cast<TmApp*>(term.get());
        std::string func_name = tm_app->func->toString();
        if (func_name == "+" || func_name == "-" || func_name == "*") {
            incre::TermType param_type = tm_app->param->getType();
            bool need_bracket = !(param_type == incre::TermType::VALUE ||
                                  param_type == incre::TermType::VAR || param_type == incre::TermType::TUPLE
                                  || param_type == incre::TermType::PROJ);
            std::cout << " ";
            if (need_bracket) std::cout << "(";
            termToHaskell(tm_app->param);
            if (need_bracket) std::cout << ")";
            std::cout << " ";
            termToHaskell(tm_app->func);
            std::cout << " ";
        }
        else {
            if (in_fix_func && func_name == post_func_name) {
              std::cout << pre_func_name;
            } else if (modified_func_name.find(func_name) != modified_func_name.end()) {
              // if the func name is reserved word, add "'" at the end
              std::cout << func_name << "'";
            } else {
              termToHaskell(tm_app->func);
            }
            incre::TermType param_type = tm_app->param->getType();
            bool need_bracket = !(param_type == incre::TermType::VALUE ||
                                  param_type == incre::TermType::VAR || param_type == incre::TermType::TUPLE
                                  || param_type == incre::TermType::PROJ);
            std::cout << " ";
            if (need_bracket) std::cout << "(";
            termToHaskell(tm_app->param);
            if (need_bracket) std::cout << ")";
        }
    } else if (term->getType() == TermType::FIX) {
        if (debug_haskell) std::cout << "[FIX]" << std::endl;
        auto* tm_fix = dynamic_cast<TmFix*>(term.get());
        if (tm_fix->content->getType() != TermType::ABS) {
            std::cout << "error: fix.content is not Term::ABS!" << std::endl;
        } else {
            auto* tm_first_content = dynamic_cast<TmAbs*>(tm_fix->content.get());
            termToHaskell(tm_first_content->content);
        }
        printSpace(floor_num_haskell);
        if (debug_haskell) std::cout << std::endl << "[FIX_END]" << std::endl;
    } else if (term->getType() == TermType::MATCH) {
        if (debug_haskell) std::cout << "[MATCH]" << std::endl;
        auto* tm_match = dynamic_cast<TmMatch*>(term.get());
        std::cout << "case ";
        termToHaskell(tm_match->def);
        std::cout << " of" << std::endl;
        floor_num_haskell++;
        for (auto &match_case : tm_match->cases) {
            printSpace(floor_num_haskell);
            patternToHaskell(match_case.first);
            std::cout << " -> ";
            floor_num_haskell++;
            auto term_type = match_case.second->getType();
            if (term_type == TermType::LET || term_type == TermType::FIX ||
                term_type == TermType::MATCH || term_type == TermType::ALIGN) {
                std::cout << std::endl;
                printSpace(floor_num_haskell);
            }
            termToHaskell(match_case.second);
            floor_num_haskell--;
            std::cout << std::endl;
        }
        floor_num_haskell--;
        printSpace(floor_num_haskell);
    } else if (term->getType() == TermType::LABEL) {
        auto* tm_label = dynamic_cast<TmLabel*>(term.get());
        auto need_bracket = _isNeedBracket(tm_label->content->getType());
        std::cout << "label ";
        if (need_bracket) std::cout << "(";
        termToHaskell(tm_label->content);
        if (need_bracket) std::cout << ")";
        std::cout << " ";
    } else if (term->getType() == TermType::UNLABEL) {
        auto* tm_unlabel = dynamic_cast<TmUnLabel*>(term.get());
        auto need_bracket = _isNeedBracket(tm_unlabel->content->getType());
        std::cout << "unlabel ";
        if (need_bracket) std::cout << "(";
        termToHaskell(tm_unlabel->content);
        if (need_bracket) std::cout << ")";
        std::cout << " ";
    } else if (term->getType() == TermType::ALIGN) {
        auto* tm_align = dynamic_cast<TmAlign*>(term.get());
        auto need_bracket = _isNeedBracket(tm_align->content->getType());
        std::cout << "align ";
        if (need_bracket) std::cout << "(";
        termToHaskell(tm_align->content);
        if (need_bracket) std::cout << ")";
        std::cout << " ";
    } else {
        LOG(FATAL) << "Unknown term";
    }
}

// 获取函数的参数
void incre::termParamToHaskell(const std::shared_ptr<TermData> &term, const bool &first_fix, const bool &first_param) {
    if (term->getType() == TermType::ABS) {
        if (debug_haskell) std::cout << "[ABS-PARAM]" << std::endl;
        auto* tm_abs = dynamic_cast<TmAbs*>(term.get());
        if (!first_param) {
            std::cout << tm_abs->name << " ";
        }
        termParamToHaskell(tm_abs->content, first_fix, false);
    } else if (term->getType() == TermType::FIX && first_fix) {
        if (debug_haskell) std::cout << "[FIX-PARAM]" << std::endl;
        auto* tm_fix = dynamic_cast<TmFix*>(term.get());
        termParamToHaskell(tm_fix->content, false, true);
    }
}

void incre::bindingToHaskell(const std::shared_ptr<BindingData> &binding) {
    if (debug_haskell) std::cout << std::endl << "[zyw: bindingToHaskell]" << std::endl;
    floor_num_haskell++;
    if (binding->getType() == BindingType::TYPE) {
        if (debug_haskell) std::cout << "[TYPE]" << std::endl;
        auto* type_binding = dynamic_cast<TypeBinding*>(binding.get());
        tyToHaskell(type_binding->type);
    } else if (binding->getType() == BindingType::TERM) {
        if (debug_haskell) std::cout << "[TERM]" << std::endl;
        auto* term_binding = dynamic_cast<TermBinding*>(binding.get());
        termParamToHaskell(term_binding->term);
        std::cout << "= " << std::endl;
        printSpace(floor_num_haskell);
        termToHaskell(term_binding->term);
        if (debug_haskell) std::cout << std::endl << "[Binding_TERM_term_BEGIN]" << std::endl;
        if (term_binding->type) {
            std::cout << ": ";
            tyToHaskell(term_binding->type);
            if (debug_haskell) std::cout << std::endl << "[Binding_TERM_type_END]" << std::endl;
        }
    } else if (binding->getType() == BindingType::VAR) {
        if (debug_haskell) std::cout << "[VAR]" << std::endl;
        auto* var_type_binding = dynamic_cast<VarTypeBinding*>(binding.get());
        tyToHaskell(var_type_binding->type);
    } else {
        LOG(FATAL) << "Unknown binding";
    }
    floor_num_haskell--;
}

// 输出binding的类型（如果有的话），用于函数的类型声明。name是函数名
void incre::bindingTyToHaskell(const std::shared_ptr<BindingData> &binding, const std::string &name) {
    if (debug_haskell) std::cout << std::endl << "[zyw: bindingTyToHaskell]" << std::endl;
    floor_num_haskell++;
    if (binding->getType() == BindingType::TYPE) {
        if (debug_haskell) std::cout << "[TYPE-getty]" << std::endl;
        auto* type_binding = dynamic_cast<TypeBinding*>(binding.get());
        std::cout << name << " :: ";
        tyToHaskell(type_binding->type);
        std::cout << std::endl;
    } else if (binding->getType() == BindingType::TERM) {
        if (debug_haskell) std::cout << "[TERM-getty]" << std::endl;
        auto* term_binding = dynamic_cast<TermBinding*>(binding.get());
        // 如果有type就print，否则可以通过自动类型推导得到，没有类型声明Haskell也可以运行
        if (term_binding->type) {
            std::cout << name << " :: ";
            tyToHaskell(term_binding->type);
            std::cout << std::endl;
        }
        // 如果是fix构成的函数，第一个参数(abs)的类型为该函数的类型
        else if (term_binding->term->getType() == TermType::FIX) {
            if (debug_haskell) std::cout << "[FIX-getty]" << std::endl;
            pre_func_name = name;
            in_fix_func = true;
            auto* fix_term = dynamic_cast<TmFix*>(term_binding->term.get());
            if (fix_term->content->getType() == TermType::ABS) {
                auto* first_abs_term = dynamic_cast<TmAbs*>(fix_term->content.get());
                post_func_name = first_abs_term->name;
                if (debug_haskell) std::cout << std::endl << "zyw: pre_func_name = " << pre_func_name << ", post_func_name = " << post_func_name << std::endl;
                std::cout << name << " :: ";
                tyToHaskell(first_abs_term->type);
                std::cout << std::endl;
            } else {
              LOG(FATAL) << "Unknown fix func";
            }
        }
    } else if (binding->getType() == BindingType::VAR) {
        if (debug_haskell) std::cout << "[VAR]" << std::endl;
        auto* var_type_binding = dynamic_cast<VarTypeBinding*>(binding.get());
        std::cout << name << " :: ";
        tyToHaskell(var_type_binding->type);
        std::cout << std::endl;
    } else {
        LOG(FATAL) << "Unknown binding";
    }
    floor_num_haskell--;
}

void incre::commandToHaskell(const std::shared_ptr<CommandData> &command) {
    floor_num_haskell = 0;
    for (auto deco: command->decorate_set) {
        std::cout << "@" << decorate2String(deco) << " ";
    }
    if (debug_haskell) std::cout << std::endl << "[zyw: commandToHaskell]" << std::endl;
    if (command->getType() == CommandType::IMPORT) {
        if (debug_haskell) std::cout << "[IMPORT]" << std::endl;
        auto* command_import = dynamic_cast<CommandImport*>(command.get());
        std::cout << command_import->name;
        for (auto& command: command_import->commands) {
            commandToHaskell(command);
        }
    } else if (command->getType() == CommandType::BIND) {
        if (debug_haskell) std::cout << "[BIND]" << std::endl;
        auto* command_bind = dynamic_cast<CommandBind*>(command.get());
        // if the func name is reserved word, add "'" at the end
        if (modified_func_name.find(command_bind->name) != modified_func_name.end()) {
          command_bind->name += "'";
        }
        bindingTyToHaskell(command_bind->binding, command_bind->name);
        std::cout << command_bind->name << " ";
        bindingToHaskell(command_bind->binding);
        std::cout << std::endl;
        pre_func_name = post_func_name = "";
        in_fix_func = false;
    } else if (command->getType() == CommandType::DEF_IND) {
        if (debug_haskell) std::cout << "[DEF_IND]" << std::endl;
        auto* command_def = dynamic_cast<CommandDefInductive*>(command.get());
        auto* ty_ind = dynamic_cast<TyInductive*>(command_def->_type.get());
        // name of the newly defined type
        std::string def_name = ty_ind->name;
        if (debug_haskell) std::cout << std::endl << "zyw: def_name = " << def_name << std::endl;
        std::cout << "data ";
        floor_num_haskell++;
        tyToHaskell(command_def->_type);
        floor_num_haskell--;
        outputDeriving(def_name);
    } else {
        LOG(FATAL) << "Unknown command";
    }
}

// some output before processing commands, read from incre-tests/pre_output.txt
void incre::preOutput() {
    std::string pre_output_path = config::KSourcePath + "incre-tests/pre_output.txt";
    std::ifstream pre_output_file(pre_output_path);
    if (pre_output_file.is_open()) {
        std::string line;
        while (std::getline(pre_output_file, line)) {
            std::cout << line << '\n';
        }
        pre_output_file.close();
    } else {
        std::cout << "Unable to open pre_output_file\n";
    }
}

// some output after processing commands, read from incre-tests/post_output.txt
void incre::postOutput(const std::vector<std::pair<Term, Data>> &io_pairs) {
    std::string post_output_path = config::KSourcePath + "incre-tests/post_output.txt";
    std::ifstream post_output_file(post_output_path);
    if (post_output_file.is_open()) {
        std::string line;
        while (std::getline(post_output_file, line)) {
            std::cout << line << '\n';
        }
        post_output_file.close();
    } else {
        std::cout << "Unable to open post_output_file\n";
    }

    /*
    example:
    let pairs = [(Nil Unit, 0)
                , ((Cons (2) (Cons (-5) (Cons (-1) (Cons (-4) (Nil Unit))))), (-8))
                ]
    */
    int l = io_pairs.size();
    for (int i = 0; i < l; ++i) {
        auto* tm_app = dynamic_cast<TmApp*>(io_pairs[i].first.get());
        auto* tm_value = dynamic_cast<TmValue*>(tm_app->param.get());
        printSpace(4);
        if (i) std::cout << ", ";
        std::cout << "((" << tm_value->data.toString() << "), " << io_pairs[i].second.toString() << ")" << std::endl;
    }
    printSpace(4);
    std::cout << "]" << std::endl;
    printSpace(1);
    std::cout << "ioPair pairs" << std::endl;
}

/*
data AExpr
  = I SymInteger
  | Var (UnionM Ident)
  | Add (UnionM AExpr) (UnionM AExpr)
  | AList List
  | If (UnionM BExpr) (UnionM AExpr) (UnionM AExpr)
  deriving stock (Generic, Show)
  deriving (Mergeable, EvaluateSym, ToCon AExpr)
    via (Default AExpr)
*/
void incre::grammarToHaskell(Grammar *grammar, int func_num,
        std::unordered_map<std::string, int>& name_to_expr_num, int& next_expr_num) {
    name_to_expr_num[grammar->start->name] = 0;
    next_expr_num = 1;
    for (auto* node: grammar->symbol_list) {
        if (name_to_expr_num.find(node->name) == name_to_expr_num.end()) {
            name_to_expr_num[node->name] = next_expr_num++;
        }
    }
    for (auto* node: grammar->symbol_list) {
        std::cout << "data Expr" << func_num << "_" << name_to_expr_num[node->name] << std::endl;
        bool flag = false;
        for (auto* rule: node->rule_list) {
            if (!flag) {
                std::cout << "  = ";
                flag = true;
            } else {
                std::cout << "  | ";
            }
            std::cout << rule->toHaskell(name_to_expr_num, next_expr_num, func_num) << std::endl;
        }
        std::cout << "  deriving stock (Generic, Show)" << std::endl
            << "  deriving (Mergeable, EvaluateSym, ToCon Expr" << name_to_expr_num[node->name] << ")"
            << std::endl << "    via (Default Expr" << name_to_expr_num[node->name] << ")" << std::endl;
        std::cout << std::endl;
    }
}

void incre::evalToHaskell(Grammar *grammar, int func_num,
        std::unordered_map<std::string, int>& name_to_expr_num) {
    for (auto* node: grammar->symbol_list) {
        std::string node_name = std::to_string(func_num);
        node_name += "_"; node_name += std::to_string(name_to_expr_num[node->name]);
        std::cout << "eval" << node_name << " :: SymIntEnv -> SymBoolEnv -> Expr"
            << node_name << " -> " << node->type->getName() << std::endl;
        for (auto* rule: node->rule_list) {
            std::cout << "eval" << node_name << " env1 env2 (";
            std::cout << ") = ";
        }
    }
}

void incre::programToHaskell(const std::shared_ptr<ProgramData> &prog, 
    const std::vector<std::pair<Term, Data>> &io_pairs, 
    incre::IncreInfo *info,
    const incre::IncreAutoLifterSolver *solver, const std::string &path) {
    construct.clear();
    std::ofstream outFile(path);
    std::streambuf *cout_buf = std::cout.rdbuf();
    if (!path.empty()) {
        std::cout.rdbuf(outFile.rdbuf());  
    }
    
    // output some content
    preOutput();

    // output def_inductive command
    for (auto &command : prog->commands) {
        if (command->getType() == CommandType::DEF_IND) {
            std::cout << std::endl;
            commandToHaskell(command);
        }
    }
    std::cout << std::endl << "------type def end------" << std::endl << std::endl
        << "------program space begin----" << std::endl;
    
    // get var for each space
    for (auto& align_info: info->align_infos) {
        std::cout << "zyw: align_info->print()" << std::endl;
        align_info->print();
    }

    // get program space
    std::vector<std::unordered_map<std::string, int> > name_to_expr_num;
    std::vector<int> next_expr_num;
    TyList final_type_list = {std::make_shared<TyTuple>((TyList){std::make_shared<TyInt>(), std::make_shared<TyInt>()})};
    for (int i = 0; i < info->align_infos.size(); ++i) {
        auto [param_list, grammar] = buildFinalGrammar(info, i, final_type_list);
        std::cout << std::endl << "Hole grammar for #" << i << std::endl;
        for (auto& param: param_list) {
            std::cout << param << " ";
        }
        std::cout << std::endl;
        name_to_expr_num.push_back(std::unordered_map<std::string, int>());
        next_expr_num.push_back(0);
        grammarToHaskell(grammar, i, name_to_expr_num[i], next_expr_num[i]);
    }

    // output eval function
    for (int i = 0; i < info->align_infos.size(); ++i) {
        auto [param_list, grammar] = buildFinalGrammar(info, i, final_type_list);
        evalToHaskell(grammar, i, name_to_expr_num[i]);
    }

    std::cout << std::endl << "------program space end----" << std::endl << std::endl
        << "------spec begin-------" << std::endl;

    // output spec function
    for (auto &command: prog->commands) {
        if (command->getType() == CommandType::BIND) {
            std::cout << std::endl;
            commandToHaskell(command);
        }
    }
    std::cout << std::endl << "------spec end-------" << std::endl << std::endl
        << "------main function-----" << std::endl;
    
    // output main function
    postOutput(io_pairs);

    if (!path.empty()) {
        std::cout.rdbuf(cout_buf);
    }
}
