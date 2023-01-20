//
// Created by pro on 2022/9/25.
//

#include "istool/incre/incre_solver.h"
#include "istool/incre/io/incre_printer.h"
#include "glog/logging.h"
#include <iostream>

using namespace incre;

IncreSolution::IncreSolution(const TyList &_compress_type_list, const TermList &_align_list):
    compress_type_list(_compress_type_list), align_list(_align_list) {
}
void IncreSolution::print() const {
    for (int i = 0; i < compress_type_list.size(); ++i) std::cout << "compress #" << i << ": " << compress_type_list[i]->toString() << std::endl;
    for (int i = 0; i < align_list.size(); ++i) {
        std::cout << "pass #" << i << ": " << std::endl;
        incre::printTerm(align_list[i]); std::cout << std::endl;
    }
}
IncreSolver::IncreSolver(IncreInfo *_info): info(_info) {}


namespace {

    Ty _rewriteType(const Ty& type, const IncreSolution& solution);
    Term _rewriteTerm(const Term& term, const IncreSolution& solution);

#define TypeHead(name) Ty _rewriteTypeCase(Ty ## name* type, const Ty& _type, const IncreSolution& solution)
#define TypeCase(name) return _rewriteTypeCase(dynamic_cast<Ty ## name*>(type.get()), type, solution)

    TypeHead(Tuple) {
        TyList fields;
        for (auto& field: type->fields) {
            fields.push_back(_rewriteType(field, solution));
        }
        return std::make_shared<TyTuple>(fields);
    }
    TypeHead(Inductive) {
        std::vector<std::pair<std::string, Ty>> cons_list;
        for (const auto& [name, cons_type]: type->constructors) {
            cons_list.emplace_back(name, _rewriteType(cons_type, solution));
        }
        return std::make_shared<TyInductive>(type->name, cons_list);
    }
    TypeHead(LabeledCompress) {
        assert(type);
        return solution.compress_type_list[type->id];
    }
    TypeHead(Arrow) {
        auto source = _rewriteType(type->source, solution);
        auto target = _rewriteType(type->target, solution);
        return std::make_shared<TyArrow>(source, target);
    }
    Ty _rewriteType(const Ty& type, const IncreSolution& solution) {
        switch (type->getType()) {
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT:
            case TyType::VAR:
                return type;
            case TyType::TUPLE: TypeCase(Tuple);
            case TyType::IND: TypeCase(Inductive);
            case TyType::COMPRESS: TypeCase(LabeledCompress);
            case TyType::ARROW: TypeCase(Arrow);
        }
    }

#define TermHead(name) Term _rewriteTermCase(Tm ## name* term, const Term& _term, const IncreSolution& solution)
#define TermCase(name) return _rewriteTermCase(dynamic_cast<Tm ## name*>(term.get()), term, solution)
#define Rewrite(name) auto name = _rewriteTerm(term->name, solution)

    TermHead(Tuple) {
        TermList fields;
        for (const auto& sub_term: term->fields) {
            fields.push_back(_rewriteTerm(sub_term, solution));
        }
        return std::make_shared<TmTuple>(fields);
    }
    TermHead(Let) {
        Rewrite(def); Rewrite(content);
        return std::make_shared<TmLet>(term->name, def, content);
    }
    TermHead(Proj) {
        Rewrite(content);
        return std::make_shared<TmProj>(content, term->id);
    }
    TermHead(Abs) {
        Rewrite(content); auto type = _rewriteType(term->type, solution);
        return std::make_shared<TmAbs>(term->name, type, content);
    }
    TermHead(App) {
        Rewrite(func); Rewrite(param);
        return std::make_shared<TmApp>(func, param);
    }
    TermHead(If) {
        Rewrite(c); Rewrite(t); Rewrite(f);
        return std::make_shared<TmIf>(c, t, f);
    }
    TermHead(Match) {
        Rewrite(def);
        std::vector<std::pair<Pattern, Term>> cases;
        for (const auto& [pattern, sub_term]: term->cases) {
            cases.emplace_back(pattern, _rewriteTerm(sub_term, solution));
        }
        return std::make_shared<TmMatch>(def, cases);
    }
    TermHead(LabeledAlign) {
        assert(term);
        return solution.align_list[term->id];
    }
    TermHead(Fix) {
        Rewrite(content);
        return std::make_shared<TmFix>(content);
    }

    Term _rewriteTerm(const Term& term, const IncreSolution& solution) {
        switch (term->getType()) {
            case TermType::TUPLE: TermCase(Tuple);
            case TermType::VAR:
            case TermType::VALUE: return term;
            case TermType::LET: TermCase(Let);
            case TermType::PROJ: TermCase(Proj);
            case TermType::APP: TermCase(App);
            case TermType::ABS: TermCase(Abs);
            case TermType::IF: TermCase(If);
            case TermType::MATCH: TermCase(Match);
            case TermType::ALIGN: TermCase(LabeledAlign);
            case TermType::LABEL:
            case TermType::UNLABEL:
                LOG(FATAL) << "Unexceptional label/unlabel while rewriting: every label/unlabel should be covered by align";
            case TermType::FIX: TermCase(Fix);
        }
    }

    Binding _rewriteBinding(BindingData* binding, const IncreSolution& solution) {
        switch (binding->getType()) {
            case BindingType::TYPE: {
                auto* tb = dynamic_cast<TypeBinding*>(binding);
                return std::make_shared<TypeBinding>(_rewriteType(tb->type, solution));
            }
            case BindingType::TERM: {
                auto* tb = dynamic_cast<TermBinding*>(binding);
                return std::make_shared<TermBinding>(_rewriteTerm(tb->term, solution));
            }
        }
    }

    Command _rewriteCommand(CommandData* command, const IncreSolution& solution) {
        switch (command->getType()) {
            case CommandType::IMPORT:
                LOG(FATAL) << "Unsupport command IMPORT";
            case CommandType::DEF_IND: {
                auto* ic = dynamic_cast<CommandDefInductive*>(command);
                return std::make_shared<CommandDefInductive>(_rewriteType(ic->_type, solution));
            }
            case CommandType::BIND: {
                auto* bc = dynamic_cast<CommandBind*>(command);
                return std::make_shared<CommandBind>(bc->name, _rewriteBinding(bc->binding.get(), solution));
            }
        }
    }
}

IncreProgram incre::rewriteWithIncreSolution(ProgramData *program, const IncreSolution &solution) {
    CommandList res;
    for (const auto& command: program->commands) {
        res.push_back(_rewriteCommand(command.get(), solution));
    }
    return std::make_shared<ProgramData>(res);
}