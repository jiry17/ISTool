//
// Created by pro on 2022/10/8.
//

#include "istool/incre/io/incre_printer.h"
#include "glog/logging.h"
#include "istool/incre/analysis/incre_instru_types.h"
#include <fstream>
#include <iostream>

using namespace incre;
bool debug = false;
bool align_mark = false;
int floor_num = 0;
bool in_func = false;
bool is_command_bind = false;

namespace {
    bool _isNeedBracket(incre::syntax::TermType def_type) {
        return !(def_type == incre::syntax::TermType::VALUE ||
                 def_type == incre::syntax::TermType::VAR || def_type == incre::syntax::TermType::TUPLE
                 || def_type == incre::syntax::TermType::PROJ);
    }

    void printSpace(int num) {
        while (num--) {
            std::cout << "  ";
        }
    }
}

namespace incre::syntax {
    void printTy(const Ty &ty) {
        if (debug) std::cout << std::endl << "[printTy] ";
        if (ty->getType() == TypeType::VAR) {
            if (debug) std::cout << "[VAR]" << std::endl;
            auto* ty_var = dynamic_cast<TyVar*>(ty.get());
            if (ty_var->is_bounded()) {
                Ty ty_var_value = std::get<Ty>(ty_var->info);
                printTy(ty_var_value);
            } else {
                std::pair<int, int> pair_value = std::get<std::pair<int, int>>(ty_var->info);
                std::cout << (char)(pair_value.first + 'a');
                // std::cout << "(" << pair_value.first << ", " << pair_value.second << ")" << std::endl;
            }
        } else if (ty->getType() == TypeType::UNIT) {
            if (debug) std::cout << "[UNIT]" << std::endl;
            std::cout << "Unit";
        } else if (ty->getType() == TypeType::BOOL) {
            if (debug) std::cout << "[BOOL]" << std::endl;
            std::cout << "Bool";
        } else if (ty->getType() == TypeType::INT) {
            if (debug) std::cout << "[INT]" << std::endl;
            std::cout << "Int";
        } else if (ty->getType() == TypeType::POLY) {
            if (debug) std::cout << "[POLY]" << std::endl;
            auto* ty_poly = dynamic_cast<TyPoly*>(ty.get());
            // int len = ty_poly->var_list.size();
            // for (int i = 0; i < len; ++i) {
            //     if (i) std::cout << " ";
            //     std::cout << (char)(ty_poly->var_list[i] + 'a');
            // }
            // std::cout << std::endl;
            printTy(ty_poly->body);
        } else if (ty->getType() == TypeType::ARR) {
            if (debug) std::cout << "[ARR]" << std::endl;
            auto* ty_arrow = dynamic_cast<TyArr*>(ty.get());
            incre::syntax::TypeType source_type = ty_arrow->inp->getType();
            bool need_bracket = !(source_type == TypeType::INT ||
                source_type == TypeType::UNIT || source_type == TypeType::BOOL
                || source_type == TypeType::VAR || source_type == TypeType::COMPRESS);
            if (need_bracket) std::cout << "(";
            printTy(ty_arrow->inp);
            if (need_bracket) std::cout << ")";
            std::cout << " -> ";
            printTy(ty_arrow->oup);
        } else if (ty->getType() == TypeType::TUPLE) {
            if (debug) std::cout << "[TUPLE]" << std::endl;
            auto* ty_tuple = dynamic_cast<TyTuple*>(ty.get());
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
        } else if (ty->getType() == TypeType::IND) {
            if (debug) std::cout << "[IND]" << std::endl;
            auto* ty_ind = dynamic_cast<TyInd*>(ty.get());
            std::cout << ty_ind->name;
            int len = ty_ind->param_list.size();
            for (int i = 0; i < len; ++i) {
                std::cout << " ";
                printTy(ty_ind->param_list[i]);
            }
            /*std::cout << ty_ind->name << " = ";
            bool flag = false;
            for (auto& ind_cons : ty_ind->constructors) {
                if (flag) {
                    std::cout << " | ";
                } else {
                    flag = true;
                }
                std::cout << ind_cons.first << " ";
                printTy(ind_cons.second);
            }*/
        } else if (ty->getType() == TypeType::COMPRESS) {
            if (debug) std::cout << "[COMPRESS]" << std::endl;
            auto* ty_compress = dynamic_cast<TyCompress*>(ty.get());
            std::cout << "Compress ";
            printTy(ty_compress->body);
        } else {
            LOG(FATAL) << "Unknown Ty";
        }
    }

    void printPattern(const Pattern &pattern) {
        if (debug) std::cout << std::endl << "[printPattern] ";
        if (pattern->getType() == PatternType::UNDERSCORE) {
            if (debug) std::cout << "[UNDERSCORE]" << std::endl;
            std::cout << "_";
        } else if (pattern->getType() == PatternType::VAR) {
            if (debug) std::cout << "[VAR]" << std::endl;
            auto* pt_var = dynamic_cast<PtVar*>(pattern.get());
            if (pt_var->body) {
                std::cout << "(";
                printPattern(pt_var->body);
                std::cout << ")@" << pt_var->name;
            } else {
                std::cout << pt_var->name;
            }
        } else if (pattern->getType() == PatternType::TUPLE) {
            if (debug) std::cout << "[TUPLE]" << std::endl;
            auto* pt_tuple = dynamic_cast<PtTuple*>(pattern.get());
            bool flag = false;
            std::cout << "{";
            for (auto& pat : pt_tuple->fields) {
                if (flag) {
                std::cout << ", ";
                printPattern(pat);
                } else {
                printPattern(pat);
                flag = true;
                }
            }
            std::cout << "}";
        } else if (pattern->getType() == PatternType::CONS) {
            if (debug) std::cout << "[CONSTRUCTOR]" << std::endl;
            auto* pt_cons = dynamic_cast<PtCons*>(pattern.get());
            std::cout << pt_cons->name << " ";
            printPattern(pt_cons->body);
        } else {
            LOG(FATAL) << "Unknown pattern";
        }
    }

    void printTerm(const Term &term) {
        if (debug) std::cout << std::endl << "[printTerm] ";
        if (term->getType() == TermType::VALUE) {
            if (debug) std::cout << "[VALUE]" << std::endl;
            auto* tm_value = dynamic_cast<TmValue*>(term.get());
            if (tm_value->v.isNull()) {
                std::cout << "Unit";
            } else {
                std::cout << tm_value->v.toString();
            }
        } else if (term->getType() == TermType::IF) {
            if (debug) std::cout << "[IF]" << std::endl;
            auto* tm_if = dynamic_cast<TmIf*>(term.get());
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
            auto* tm_var = dynamic_cast<TmVar*>(term.get());
            std::cout << tm_var->name;
            if (debug) std::cout << std::endl << "[Term_VAR_END]" << std::endl;
        } else if (term->getType() == TermType::PRIMARY) {
            if (debug) std::cout << "[PRIMARY]" << std::endl;
            auto* tm_primary = dynamic_cast<TmPrimary*>(term.get());
            int len = tm_primary->params.size();
            if (len == 1) {
                if (tm_primary->op_name == "neg") {
                    std::cout << "-";
                    printTerm(tm_primary->params[0]);
                } else {
                    std::cout << tm_primary->op_name << " ";
                    printTerm(tm_primary->params[0]);
                }
            } else if (len == 2) {
                printTerm(tm_primary->params[0]);
                std::cout << " " << tm_primary->op_name << " ";
                printTerm(tm_primary->params[1]);
            } else {
                std::cout << tm_primary->op_name;
                for (int i = 0; i < len; ++i) {
                    std::cout << " ";
                    printTerm(tm_primary->params[i]);
                }
            }
        } else if (term->getType() == TermType::APP) {
            if (debug) std::cout << "[APP]" << std::endl;
            auto* tm_app = dynamic_cast<TmApp*>(term.get());
            printTerm(tm_app->func);
            TermType param_type = tm_app->param->getType();
            bool need_bracket = !(param_type == TermType::VALUE ||
            param_type == TermType::VAR || param_type == TermType::TUPLE
            || param_type == TermType::PROJ);
            std::cout << " ";
            if (need_bracket) std::cout << "(";
            printTerm(tm_app->param);
            if (need_bracket) std::cout << ")";
        } else if (term->getType() == TermType::TUPLE) {
            if (debug) std::cout << "[TUPLE]" << std::endl;
            auto* tm_tuple = dynamic_cast<TmTuple*>(term.get());
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
            auto* tm_proj = dynamic_cast<TmProj*>(term.get());
            TermType content_type = tm_proj->body->getType();
            bool need_bracket = !(content_type == TermType::VALUE || content_type == TermType::VAR || content_type == TermType::TUPLE || content_type == TermType::PROJ);
            if (need_bracket) std::cout << "(";
            printTerm(tm_proj->body);
            if (need_bracket) std::cout << ")";
            std::cout << "." << tm_proj->id << "/" << tm_proj->size;
        } else if (term->getType() == TermType::FUNC) {
            if (debug) std::cout << "[FUNC]" << std::endl;
            auto* tm_func = dynamic_cast<TmFunc*>(term.get());
            if (!in_func) {
                std::cout << "fun " << tm_func->name;
                in_func = true;
            } else {
                std::cout << " " << tm_func->name;
            }
            if (tm_func->body->getType() != TermType::FUNC) {
                if (is_command_bind) {
                    std::cout << " = ";
                    is_command_bind = false;
                    in_func = false;
                } else {
                    std::cout << " -> ";
                    in_func = false;
                }
            }
            printTerm(tm_func->body);
        } else if (term->getType() == TermType::LET) {
            if (debug) std::cout << "[LET]" << std::endl;
            auto* tm_let = dynamic_cast<TmLet*>(term.get());
            TermType def_type = tm_let->def->getType();
            bool need_bracket = !(def_type == TermType::VALUE ||
            def_type == TermType::VAR || def_type == TermType::TUPLE
            || def_type == TermType::PROJ);
            std::cout << "let " << tm_let->name << " = ";
            if (need_bracket) std::cout << "(";
            printTerm(tm_let->def);
            if (need_bracket) std::cout << ")";
            std::cout << " in " << std::endl;
            floor_num++; printSpace(floor_num);
            printTerm(tm_let->body);
            floor_num--;
        }
        // else if (term->getType() == TermType::ABS) {
        //     if (debug) std::cout << "[ABS]" << std::endl;
        //     auto* tm_abs = dynamic_cast<TmAbs*>(term.get());
        //     std::cout << "\\" << tm_abs->name << ": ";
        //     printTy(tm_abs->type);
        //     std::cout << ". ";
        //     if (tm_abs->content->getType() != TermType::ABS) {
        //         std::cout << std::endl;
        //         printSpace(floor_num);
        //     }
        //     printTerm(tm_abs->content);
        // } else if (term->getType() == TermType::FIX) {
        //     if (debug) std::cout << "[FIX]" << std::endl;
        //     auto* tm_fix = dynamic_cast<TmFix*>(term.get());
        //     std::cout << "fix (" << std::endl;
        //     printSpace(floor_num);
        //     printTerm(tm_fix->content);
        //     printSpace(floor_num-1);
        //     std::cout << ")";
        //     if (debug) std::cout << std::endl << "[FIX_END]" << std::endl;
        // }
        else if (term->getType() == TermType::MATCH) {
            if (debug) std::cout << "[MATCH]" << std::endl;
            auto* tm_match = dynamic_cast<TmMatch*>(term.get());
            std::cout << "match ";
            printTerm(tm_match->def);
            std::cout << " with" << std::endl;
            bool flag = false;
            floor_num++;
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
                // if (term_type == TermType::LET || term_type == TermType::FIX || term_type == TermType::MATCH || term_type == TermType::ALIGN) {
                //     std::cout << std::endl;
                //     printSpace(floor_num);
                // }
                if (term_type == TermType::LET || term_type == TermType::MATCH) {
                    std::cout << std::endl;
                    printSpace(floor_num);
                }
                printTerm(match_case.second);
                floor_num--;
                std::cout << std::endl;
            }
            floor_num--;
            printSpace(floor_num);
        } else if (term->getType() == TermType::CONS) {
            if (debug) std::cout << "[CONS]" << std::endl;
            auto* tm_cons = dynamic_cast<TmCons*>(term.get());
            std::cout << tm_cons->cons_name << " ";
            printTerm(tm_cons->body);
        } else if (term->getType() == TermType::LABEL) {
            auto* tm_label = dynamic_cast<TmLabel*>(term.get());
            auto* labeled_label = dynamic_cast<TmLabeledLabel*>(term.get());
            auto need_bracket = _isNeedBracket(tm_label->body->getType());
            std::cout << "label";
            if (labeled_label) std::cout << "@" << labeled_label->id << " "; else std::cout << " ";
            if (need_bracket) std::cout << "(";
            printTerm(tm_label->body);
            if (need_bracket) std::cout << ")";
            std::cout << " ";
        } else if (term->getType() == TermType::UNLABEL) {
            auto* tm_unlabel = dynamic_cast<TmUnlabel*>(term.get());
            auto need_bracket = _isNeedBracket(tm_unlabel->body->getType());
            std::cout << "unlabel ";
            if (need_bracket) std::cout << "(";
            printTerm(tm_unlabel->body);
            if (need_bracket) std::cout << ")";
        } else if (term->getType() == TermType::REWRITE) {
            auto* tm_rewrite = dynamic_cast<TmRewrite*>(term.get());
            std::cout << "rewrite (";
            printTerm(tm_rewrite->body);
            std::cout << ")";
        }
        // else if (term->getType() == TermType::ALIGN) {
        //     if (align_mark) {
        //         auto *tm_align = dynamic_cast<TmAlign *>(term.get());
        //         std::cout << "<mark>";
        //         align_mark = false;
        //         printTerm(tm_align->content);
        //         align_mark = true;
        //         std::cout << "</mark>";
        //     } else {
        //         auto *tm_align = dynamic_cast<TmAlign *>(term.get());
        //         auto *labeled_align = dynamic_cast<TmLabeledAlign *>(term.get());
        //         auto need_bracket = _isNeedBracket(tm_align->content->getType());
        //         std::cout << "align";
        //         if (labeled_align) std::cout << "@" << labeled_align->id;
        //         std::cout << " ";
        //         if (need_bracket) std::cout << "(";
        //         printTerm(tm_align->content);
        //         if (need_bracket) std::cout << ")";
        //         std::cout << " ";
        //     }
        // }
        else {
            LOG(FATAL) << "Unknown term";
        }
    }

    void printBinding(const std::shared_ptr<Binding> &binding) {
        if (debug) std::cout << std::endl << "[printBinding] ";
        floor_num++;
        if (binding->is_cons) {
            std::cout << "type: ";
            printTy(binding->type);
            std::cout << std::endl;
        } else {
            std::cout << "data: " << binding->data.toString() << std::endl;
        }
        // if (binding->getType() == BindingType::TYPE) {
        //     if (debug) std::cout << "[TYPE]" << std::endl;
        //     std::cout << " = ";
        //     auto* type_binding = dynamic_cast<TypeBinding*>(binding.get());
        //     printTy(type_binding->type);
        // } else if (binding->getType() == BindingType::TERM) {
        //     if (debug) std::cout << "[TERM]" << std::endl;
        //     std::cout << " = ";
        //     auto* term_binding = dynamic_cast<TermBinding*>(binding.get());
        //     printTerm(term_binding->term);
        //     if (debug) std::cout << std::endl << "[Binding_TERM_term_END]" << std::endl;
        //     if (term_binding->type) {
        //         std::cout << ": ";
        //         printTy(term_binding->type);
        //         if (debug) std::cout << std::endl << "[Binding_TERM_type_END]" << std::endl;
        //     }
        // } else if (binding->getType() == BindingType::VAR) {
        //     if (debug) std::cout << "[VAR]" << std::endl;
        //     std::cout << " : ";
        //     auto* var_type_binding = dynamic_cast<VarTypeBinding*>(binding.get());
        //     printTy(var_type_binding->type);
        // } else {
        //     LOG(FATAL) << "Unknown binding";
        // }
        floor_num--;
    }
}

namespace incre{
    void printCommand(const Command &command) {
        floor_num = 0;
        in_func = false;
        is_command_bind = false;
        // for (auto deco: command->decorate_set) {
        //     std::cout << "@" << decorate2String(deco) << " ";
        // }
        if (debug) std::cout << std::endl << "[printCommand] ";
        if (command->getType() == CommandType::BIND_TERM) {
            if (debug) std::cout << "[BIND_TERM]" << std::endl;
            is_command_bind = true;
            auto* command_bind = dynamic_cast<CommandBindTerm*>(command.get());
            if (command_bind->is_func) {
                std::cout << "fun ";
                in_func = true;
            }
            std::cout << command_bind->name;
            if (!command_bind->is_func) {
                std::cout << " = ";
                is_command_bind = false;
                in_func = false;
            }
            incre::syntax::printTerm(command_bind->term);
            std::cout << ";" << std::endl;
        } else if (command->getType() == CommandType::DEF_IND) {
            if (debug) std::cout << "[DEF_IND]" << std::endl;
            auto* command_def = dynamic_cast<CommandDef*>(command.get());
            std::cout << "data " << command_def->name;
            if (command_def->cons_list[0].second->getType() == incre::syntax::TypeType::POLY) {
                auto* ty_poly = dynamic_cast<incre::syntax::TyPoly*>(command_def->cons_list[0].second.get());
                int num = ty_poly->var_list.size();
                for (int i = 0; i < num; ++i) {
                    std::cout << " " << (char)(i + 'a');
                }
            }
            std::cout << " = ";
            for (int i = 0; i < command_def->cons_list.size(); ++i) {
                if (i) std::cout << " | ";
                std::cout << command_def->cons_list[i].first << " ";
                if (command_def->cons_list[i].second->getType() == incre::syntax::TypeType::POLY) {
                    auto* ty_poly = dynamic_cast<incre::syntax::TyPoly*>(command_def->cons_list[i].second.get());
                    if (ty_poly->body->getType() == incre::syntax::TypeType::ARR) {
                        auto* ty_arr = dynamic_cast<incre::syntax::TyArr*>(ty_poly->body.get());
                        if (ty_arr->inp->getType() == incre::syntax::TypeType::TUPLE) {
                            auto* ty_tuple = dynamic_cast<incre::syntax::TyTuple*>(ty_arr->inp.get());
                            int len = ty_tuple->fields.size();
                            for (int i = 0; i < len; ++i) {
                                if (i) std::cout << " * ";
                                printTy(ty_tuple->fields[i]);
                            }
                        } else {
                            printTy(ty_arr->inp);
                        }
                    }
                }
                // printTy(command_def->cons_list[i].second);
            }
            std::cout << ";" << std::endl;
        } else if (command->getType() == CommandType::DECLARE) {
            if (debug) std::cout << "[DECLARE]" << std::endl;
            auto* command_declare = dynamic_cast<CommandDeclare*>(command.get());
            std::cout << "Declare ";
            incre::syntax::printTy(command_declare->type);
            std::cout << std::endl;
        } else {
            LOG(FATAL) << "Unknown command";
        }
    }

    void printProgram(const IncreProgram &prog, const std::string &path, bool is_align_mark) {
        align_mark = is_align_mark;
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
        align_mark = false;
    }
}

