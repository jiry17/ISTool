//
// Created by pro on 2022/11/22.
//

#include "istool/incre/language/incre.h"
#include "glog/logging.h"

using namespace incre;

namespace {
    typedef std::unordered_map<std::string, bool> CompressContext;

    bool _isTmp(const std::string& name, const std::vector<std::string>& tmp_list) {
        for (auto& tmp_name: tmp_list) if (name == tmp_name) return true;
        return false;
    }


    bool _isTypeRelatedToCompress(TyData* type, const CompressContext& ctx, std::vector<std::string>& tmp_names);

#define TypeHead(name) bool _isTypeRelatedToCompressCase(Ty ## name* type, const CompressContext& ctx, std::vector<std::string>& tmp_names)
#define TypeCase(name) return _isTypeRelatedToCompressCase(dynamic_cast<Ty ## name*>(type), ctx, tmp_names)

    TypeHead(Var) {
        return !_isTmp(type->name, tmp_names) && ctx.find(type->name)->second;
    }
    TypeHead(Tuple) {
        for (auto& field: type->fields) {
            if (_isTypeRelatedToCompress(field.get(), ctx, tmp_names)) return true;
        }
        return false;
    }
    TypeHead(Arrow) {
        return _isTypeRelatedToCompress(type->source.get(), ctx, tmp_names) || _isTypeRelatedToCompress(type->target.get(), ctx, tmp_names);
    }
    TypeHead(Inductive) {
        tmp_names.push_back(type->name);
        for (auto& [name, ctype]: type->constructors) {
            if (_isTypeRelatedToCompress(ctype.get(), ctx, tmp_names)) {
                tmp_names.pop_back(); return true;
            }
        }
        tmp_names.pop_back(); return false;
    }

    bool _isTypeRelatedToCompress(TyData* type, const CompressContext& ctx, std::vector<std::string>& tmp_names) {
        switch (type->getType()) {
            case TyType::INT:
            case TyType::UNIT:
            case TyType::BOOL: return false;
            case TyType::VAR: TypeCase(Var);
            case TyType::TUPLE: TypeCase(Tuple);
            case TyType::ARROW: TypeCase(Arrow);
            case TyType::IND: TypeCase(Inductive);
            case TyType::COMPRESS: return true;
        }
    }
    bool _isTypeRelatedToCompress(TyData* type, const CompressContext& ctx) {
        std::vector<std::string> tmp_names;
        return _isTypeRelatedToCompress(type, ctx, tmp_names);
    }

    void _patternNames(PatternData* pt, std::vector<std::string>& res) {
        switch (pt->getType()) {
            case PatternType::TUPLE: {
                auto* tp = dynamic_cast<PtTuple*>(pt);
                for (auto& sub: tp->pattern_list) {
                    _patternNames(sub.get(), res);
                }
                return;
            }
            case PatternType::VAR: {
                auto* vp = dynamic_cast<PtVar*>(pt);
                res.push_back(vp->name);
                return;
            }
            case PatternType::CONSTRUCTOR: {
                auto* cp = dynamic_cast<PtConstructor*>(pt);
                _patternNames(cp->pattern.get(), res);
                return;
            }
            case PatternType::UNDER_SCORE: {
                return;
            }
        }
    }

    std::vector<std::string> _patternNames(PatternData* pt) {
        std::vector<std::string> res;
        _patternNames(pt, res);
        return res;
    }

    bool _isTermRelatedToCompress(TermData* term, const CompressContext& ctx, std::vector<std::string>& tmp_names);

#define TermHead(name) bool _isTermRelatedToCompressCase(Tm ## name* term, const CompressContext& ctx, std::vector<std::string>& tmp_names)
#define TermCase(name) return _isTermRelatedToCompressCase(dynamic_cast<Tm ## name*>(term), ctx, tmp_names)

    TermHead(Tuple) {
        for (auto& field: term->fields) {
            if (_isTermRelatedToCompress(field.get(), ctx, tmp_names)) return true;
        }
        return false;
    }
    TermHead(Var) {
        return !_isTmp(term->name, tmp_names) && ctx.find(term->name)->second;
    }
    TermHead(Match) {
        if (_isTermRelatedToCompress(term->def.get(), ctx, tmp_names)) {
            return true;
        }
        auto res = false;
        for (auto& [pt, sub_term]: term->cases) {
            std::vector<std::string> names = _patternNames(pt.get());
            for (const auto& name: names) tmp_names.push_back(name);
            res |= _isTermRelatedToCompress(sub_term.get(), ctx, tmp_names);
            for (const auto& _: names) tmp_names.pop_back();
            if (res) return true;
        }
        return false;
    }
    TermHead(Abs) {
        if (_isTypeRelatedToCompress(term->type.get(), ctx, tmp_names)) return true;
        tmp_names.push_back(term->name);
        auto res = _isTermRelatedToCompress(term->content.get(), ctx, tmp_names);
        tmp_names.pop_back();
        return res;
    }
    TermHead(Proj) {
        return _isTermRelatedToCompress(term->content.get(), ctx, tmp_names);
    }
    TermHead(App) {
        return _isTermRelatedToCompress(term->func.get(), ctx, tmp_names) || _isTermRelatedToCompress(term->param.get(), ctx, tmp_names);
    }
    TermHead(Let) {
        if (_isTermRelatedToCompress(term->def.get(), ctx, tmp_names)) return true;
        tmp_names.push_back(term->name);
        auto res = _isTermRelatedToCompress(term->content.get(), ctx, tmp_names);
        tmp_names.pop_back();
        return res;
    }
    TermHead(Fix) {
        return _isTermRelatedToCompress(term->content.get(), ctx, tmp_names);
    }
    TermHead(If) {
        for (auto& sub_term: {term->t, term->c, term->f}) {
            if (_isTermRelatedToCompress(sub_term.get(), ctx, tmp_names)) return true;
        }
        return false;
    }
    bool _isTermRelatedToCompress(TermData* term, const CompressContext& ctx, std::vector<std::string>& tmp_names) {
        switch (term->getType()) {
            case TermType::VALUE: return false;
            case TermType::PASS:
            case TermType::CREATE: return true;
            case TermType::TUPLE: TermCase(Tuple);
            case TermType::VAR: TermCase(Var);
            case TermType::MATCH: TermCase(Match);
            case TermType::ABS: TermCase(Abs);
            case TermType::APP: TermCase(App);
            case TermType::PROJ: TermCase(Proj);
            case TermType::LET: TermCase(Let);
            case TermType::FIX: TermCase(Fix);
            case TermType::IF: TermCase(If);
        }
    }

    bool _isTermRelatedToCompress(TermData* term, const CompressContext& ctx) {
        std::vector<std::string> tmp_names;
        return _isTermRelatedToCompress(term, ctx, tmp_names);
    }

    bool _processBindingCompress(BindingData* binding, const CompressContext& ctx) {
        switch (binding->getType()) {
            case BindingType::TYPE: {
                auto* tb = dynamic_cast<TypeBinding*>(binding);
                return _isTypeRelatedToCompress(tb->type.get(), ctx);
            }
            case BindingType::TERM: {
                auto* tb = dynamic_cast<TermBinding*>(binding);
                return _isTermRelatedToCompress(tb->term.get(), ctx);
            }
        }
    }
}

CompressContext incre::compressRelatedNames(const IncreProgram &program) {
    CompressContext compress_ctx;
    for (auto& command: program->commands) {
        switch (command->getType()) {
            case CommandType::IMPORT:
                LOG(FATAL) << "Unsupported command IMPORT";
            case CommandType::DEF_IND: {
                auto* cd = dynamic_cast<CommandDefInductive*>(command.get());
                bool res = _isTypeRelatedToCompress(cd->type, compress_ctx);
                compress_ctx[cd->type->name] = res;
                for (const auto& [name, _]: cd->type->constructors) {
                    compress_ctx[name] = res;
                }
                break;
            }
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                compress_ctx[cb->name] = _processBindingCompress(cb->binding.get(), compress_ctx);
                break;
            }
        }
    }
    return compress_ctx;
}