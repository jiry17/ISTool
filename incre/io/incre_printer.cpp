//
// Created by pro on 2022/10/8.
//

#include "istool/incre/io/incre_printer.h"
#include "istool/incre/language/incre.h"
#include "glog/logging.h"
#include <fstream>
#include <iostream>

using namespace incre;
bool debug = false;
int floor_num = 0;

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
}

void incre::printTy(const std::shared_ptr<TyData> &ty) {
    if (debug) std::cout << std::endl << "[zyw: printTy] ";
    if (ty->getType() == TyType::INT) {
      if (debug) std::cout << std::endl;
      std::cout << "Int";
    } else if (ty->getType() == TyType::VAR) {
      if (debug) std::cout << "[VAR]" << std::endl;
      std::shared_ptr<TyVar> ty_var =
              std::dynamic_pointer_cast<TyVar>(ty);
      std::cout << ty_var->name;
    } else if (ty->getType() == TyType::UNIT) {
      if (debug) std::cout << std::endl;
      std::cout << "Unit";
    } else if (ty->getType() == TyType::BOOL) {
      if (debug) std::cout << std::endl;
      std::cout << "Bool";
    } else if (ty->getType() == TyType::TUPLE) {
      if (debug) std::cout << "[TUPLE]" << std::endl;
      std::shared_ptr<TyTuple> ty_tuple =
              std::dynamic_pointer_cast<TyTuple>(ty);
      std::cout << "{";
      bool flag = false;
      for (auto& tuple_field : ty_tuple->fields) {
        if (flag) {
          std::cout << ", ";
          printTy(tuple_field);
        } else {
          printTy(tuple_field);
          flag = true;
        }
      }
      std::cout << "}";
    } else if (ty->getType() == TyType::IND) {
      if (debug) std::cout << "[IND]" << std::endl;
      std::shared_ptr<TyInductive> ty_ind =
              std::dynamic_pointer_cast<TyInductive>(ty);
      std::cout << ty_ind->name << " = ";
      bool flag = false;
      for (auto& ind_cons : ty_ind->constructors) {
        if (flag) {
          std::cout << " | ";
        } else {
          flag = true;
        }
        std::cout << ind_cons.first << " ";
        printTy(ind_cons.second);
      }
    } else if (ty->getType() == TyType::COMPRESS) {
      if (debug) std::cout << "[COMPRESS]" << std::endl;
      std::shared_ptr<TyCompress> ty_compress =
              std::dynamic_pointer_cast<TyCompress>(ty);
      std::cout << "Compress ";
      printTy(ty_compress->content);
    } else if (ty->getType() == TyType::ARROW) {
      if (debug) std::cout << "[ARROW]" << std::endl;
      std::shared_ptr<TyArrow> ty_arrow =
              std::dynamic_pointer_cast<TyArrow>(ty);
      incre::TyType source_type = ty_arrow->source->getType();
      bool need_bracket = !(source_type == incre::TyType::INT ||
        source_type == incre::TyType::UNIT || source_type == incre::TyType::BOOL
        || source_type == incre::TyType::VAR ||  source_type == incre::TyType::COMPRESS);
      if (need_bracket) std::cout << "(";
      printTy(ty_arrow->source);
      if (need_bracket) std::cout << ")";
      std::cout << " -> ";
      printTy(ty_arrow->target);
    } else {
      LOG(FATAL) << "Unknown Ty";
    }
}

void incre::printPattern(const std::shared_ptr<PatternData> &pattern) {
    if (debug) std::cout << std::endl << "[zyw: printPattern]" << std::endl;
    if (pattern->getType() == PatternType::UNDER_SCORE) {
      if (debug) std::cout << std::endl;
        std::cout << "_";
    } else if (pattern->getType() == PatternType::VAR) {
      if (debug) std::cout << "[VAR]" << std::endl;
        std::shared_ptr<PtVar> pt_var =
                std::dynamic_pointer_cast<PtVar>(pattern);
        std::cout << pt_var->name;
    } else if (pattern->getType() == PatternType::TUPLE) {
      if (debug) std::cout << "[TUPLE]" << std::endl;
      std::shared_ptr<PtTuple> pt_tuple =
              std::dynamic_pointer_cast<PtTuple>(pattern);
      bool flag = false;
      std::cout << "{";
      for (auto& pat : pt_tuple->pattern_list) {
        if (flag) {
          std::cout << ", ";
          printPattern(pat);
        } else {
          printPattern(pat);
          flag = true;
        }
      }
      std::cout << "}";
    } else if (pattern->getType() == PatternType::CONSTRUCTOR) {
      if (debug) std::cout << "[CONSTRUCTOR]" << std::endl;
        std::shared_ptr<PtConstructor> pt_cons =
                std::dynamic_pointer_cast<PtConstructor>(pattern);
        std::cout << pt_cons->name << " ";
        printPattern(pt_cons->pattern);
    } else {
        LOG(FATAL) << "Unknown pattern";
    }
}

void incre::printTerm(const std::shared_ptr<TermData> &term) {
    if (debug) std::cout << std::endl << "[zyw: printTerm]" << std::endl;
    if (term->getType() == TermType::VALUE) {
      if (debug) std::cout << "[VALUE]" << std::endl;
        std::shared_ptr<TmValue> tm_value =
                std::dynamic_pointer_cast<TmValue>(term);
        if (tm_value->data.isNull()) {
          std::cout << "unit";
        } else {
          std::cout << tm_value->data.toString();
        }
    } else if (term->getType() == TermType::IF) {
      if (debug) std::cout << "[IF]" << std::endl;
        std::shared_ptr<TmIf> tm_if =
                std::dynamic_pointer_cast<TmIf>(term);
        std::cout << "if (";
        printTerm(tm_if->c);
        std::cout << ") then ";
        printTerm(tm_if->t);
        std::cout << std::endl;
        printSpace(floor_num);
        std::cout << "else ";
        printTerm(tm_if->f);
    } else if (term->getType() == TermType::VAR) {
      if (debug) std::cout << "[VAR]" << std::endl;
        std::shared_ptr<TmVar> tm_var =
                std::dynamic_pointer_cast<TmVar>(term);
        std::cout << tm_var->name;
        if (debug) std::cout << std::endl << "[Term_VAR_END]" << std::endl;
    } else if (term->getType() == TermType::LET) {
      if (debug) std::cout << "[LET]" << std::endl;
        std::shared_ptr<TmLet> tm_let =
                std::dynamic_pointer_cast<TmLet>(term);
        incre::TermType def_type = tm_let->def->getType();
        bool need_bracket = !(def_type == incre::TermType::VALUE ||
          def_type == incre::TermType::VAR || def_type == incre::TermType::TUPLE
          || def_type == incre::TermType::PROJ);
        std::cout << "let " << tm_let->name << " = ";
        if (need_bracket) std::cout << "(";
        printTerm(tm_let->def);
        if (need_bracket) std::cout << ")";
        std::cout << " in " << std::endl;
        floor_num++; printSpace(floor_num);
        printTerm(tm_let->content);
        floor_num--;
    } else if (term->getType() == TermType::TUPLE) {
      if (debug) std::cout << "[TUPLE]" << std::endl;
        std::shared_ptr<TmTuple> tm_tuple =
                std::dynamic_pointer_cast<TmTuple>(term);
        bool flag = false;
        std::cout << "{";
        for (auto& tuple_field : tm_tuple->fields) {
            if (flag) {
                std::cout << ", ";
                printTerm(tuple_field);
            } else {
                printTerm(tuple_field);
                flag = true;
            }
        }
        std::cout << "}";
    } else if (term->getType() == TermType::PROJ) {
      if (debug) std::cout << "[PROJ]" << std::endl;
        std::shared_ptr<TmProj> tm_proj =
                std::dynamic_pointer_cast<TmProj>(term);
        incre::TermType content_type = tm_proj->content->getType();
        bool need_bracket = !(content_type == incre::TermType::VALUE ||
          content_type == incre::TermType::VAR || content_type == incre::TermType::TUPLE
          || content_type == incre::TermType::PROJ);
        if (need_bracket) std::cout << "(";
        printTerm(tm_proj->content);
        if (need_bracket) std::cout << ")";
        std::cout << "." << tm_proj->id;
    } else if (term->getType() == TermType::ABS) {
      if (debug) std::cout << "[ABS]" << std::endl;
        std::shared_ptr<TmAbs> tm_abs =
                std::dynamic_pointer_cast<TmAbs>(term);
        std::cout << "\\" << tm_abs->name << ": ";
        printTy(tm_abs->type);
        std::cout << ". ";
        if (tm_abs->content->getType() != incre::TermType::ABS) {
            std::cout << std::endl;
            printSpace(floor_num);
        }
        printTerm(tm_abs->content);
    } else if (term->getType() == TermType::APP) {
      if (debug) std::cout << "[APP]" << std::endl;
        std::shared_ptr<TmApp> tm_app =
                std::dynamic_pointer_cast<TmApp>(term);
        printTerm(tm_app->func);
        incre::TermType param_type = tm_app->param->getType();
        bool need_bracket = !(param_type == incre::TermType::VALUE ||
          param_type == incre::TermType::VAR || param_type == incre::TermType::TUPLE
          || param_type == incre::TermType::PROJ);
        std::cout << " ";
        if (need_bracket) std::cout << "(";
        printTerm(tm_app->param);
        if (need_bracket) std::cout << ")";
    } else if (term->getType() == TermType::FIX) {
      if (debug) std::cout << "[FIX]" << std::endl;
        std::shared_ptr<TmFix> tm_fix =
                std::dynamic_pointer_cast<TmFix>(term);
        std::cout << "fix (" << std::endl;
        printSpace(floor_num);
        printTerm(tm_fix->content);
        printSpace(floor_num-1);
        std::cout << ")";
      if (debug) std::cout << std::endl << "[FIX_END]" << std::endl;
    } else if (term->getType() == TermType::MATCH) {
        if (debug) std::cout << "[MATCH]" << std::endl;
        std::shared_ptr<TmMatch> tm_match =
                std::dynamic_pointer_cast<TmMatch>(term);
        std::cout << "match ";
        printTerm(tm_match->def);
        std::cout << " with" << std::endl;
        bool flag = false;
        for (auto &match_case : tm_match->cases) {
            printSpace(floor_num);
            if (flag) {
                std::cout << "| ";
            } else {
                std::cout << "  ";
                flag = true;
            }
            printPattern(match_case.first);
            std::cout << " -> ";
            floor_num++;
            auto term_type = match_case.second->getType();
            if (term_type == TermType::LET || term_type == TermType::FIX ||
                term_type == TermType::MATCH || term_type == TermType::ALIGN) {
                std::cout << std::endl;
                printSpace(floor_num);
            }
            printTerm(match_case.second);
            floor_num--;
            std::cout << std::endl;
        }
        printSpace(floor_num);
        std::cout << "end" << std::endl;
    } else if (term->getType() == TermType::LABEL) {
        auto* tm_label = dynamic_cast<TmLabel*>(term.get());
        auto need_bracket = _isNeedBracket(tm_label->content->getType());
        std::cout << "label ";
        if (need_bracket) std::cout << "(";
        printTerm(tm_label->content);
        if (need_bracket) std::cout << ")";
        std::cout << " ";
    } else if (term->getType() == TermType::UNLABEL) {
        auto* tm_unlabel = dynamic_cast<TmUnLabel*>(term.get());
        auto need_bracket = _isNeedBracket(tm_unlabel->content->getType());
        std::cout << "unlabel ";
        if (need_bracket) std::cout << "(";
        printTerm(tm_unlabel->content);
        if (need_bracket) std::cout << ")";
        std::cout << " ";
    } else if (term->getType() == TermType::ALIGN) {
        auto* tm_align = dynamic_cast<TmAlign*>(term.get());
        auto need_bracket = _isNeedBracket(tm_align->content->getType());
        std::cout << "align ";
        if (need_bracket) std::cout << "(";
        printTerm(tm_align->content);
        if (need_bracket) std::cout << ")";
        std::cout << " ";
    } else {
        LOG(FATAL) << "Unknown term";
    }
}

void incre::printBinding(const std::shared_ptr<BindingData> &binding) {
    if (debug) std::cout << std::endl << "[zyw: printBinding]" << std::endl;
    floor_num++;
    if (binding->getType() == BindingType::TYPE) {
        if (debug) std::cout << "[TYPE]" << std::endl;
        std::shared_ptr<TypeBinding> type_binding =
                std::dynamic_pointer_cast<TypeBinding>(binding);
        printTy(type_binding->type);
    } else if (binding->getType() == BindingType::TERM) {
        if (debug) std::cout << "[TERM]" << std::endl;
        std::shared_ptr<TermBinding> term_binding =
                std::dynamic_pointer_cast<TermBinding>(binding);
        printTerm(term_binding->term);
        if (debug) std::cout << std::endl << "[Binding_TERM_term_END]" << std::endl;
        if (term_binding->type) {
            std::cout << ": ";
            printTy(term_binding->type);
            if (debug) std::cout << std::endl << "[Binding_TERM_type_END]" << std::endl;
        }
    } else if (binding->getType() == BindingType::VAR) {
        if (debug) std::cout << "[VAR]" << std::endl;
        std::shared_ptr<VarTypeBinding> var_type_binding =
                std::dynamic_pointer_cast<VarTypeBinding>(binding);
        printTy(var_type_binding->type);
    } else {
        LOG(FATAL) << "Unknown binding";
    }
    floor_num--;
}

void incre::printCommand(const std::shared_ptr<CommandData> &command) {
    floor_num = 0;
    if (debug) std::cout << std::endl << "[zyw: printCommand]" << std::endl;
    if (command->getType() == CommandType::IMPORT) {
      if (debug) std::cout << "[IMPORT]" << std::endl;
        std::shared_ptr<CommandImport> command_import =
                std::dynamic_pointer_cast<CommandImport>(command);
        std::cout << command_import->name;
        for (auto& command: command_import->commands) {
            printCommand(command);
        }
        std::cout << ";" << std::endl;
    } else if (command->getType() == CommandType::BIND) {
      if (debug) std::cout << "[BIND]" << std::endl;
        std::shared_ptr<CommandBind> command_bind =
                std::dynamic_pointer_cast<CommandBind>(command);
        std::cout << command_bind->name << " = ";
        printBinding(command_bind->binding);
        std::cout << ";" << std::endl;
    } else if (command->getType() == CommandType::DEF_IND) {
      if (debug) std::cout << "[DEF_IND]" << std::endl;
        std::shared_ptr<CommandDefInductive> command_def =
                std::dynamic_pointer_cast<CommandDefInductive>(command);
        std::cout << "Inductive ";
        printTy(command_def->_type);
        std::cout << ";" << std::endl;
    } else {
        LOG(FATAL) << "Unknown command";
    }
}

void incre::printProgram(const std::shared_ptr<ProgramData> &prog, const std::string &path) {
    if (!path.empty()) {
        std::ofstream outFile(path);
        std::streambuf *cout_buf = std::cout.rdbuf();
        std::cout.rdbuf(outFile.rdbuf());

        for (auto &command : prog->commands) {
            std::cout << std::endl;
            printCommand(command);
        }

        std::cout.rdbuf(cout_buf);
        outFile.close();
    } else {
        for (auto& command: prog->commands) {
            std::cout << std::endl;
            printCommand(command);
        }
    }
}
