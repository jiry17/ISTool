//
// Created by pro on 2022/9/23.
//

#include "istool/incre/analysis/incre_instru_runtime.h"
#include "glog/logging.h"

using namespace incre;

IncreExampleData::IncreExampleData(int _tau_id, const std::unordered_map<std::string, Data> &_inputs, const Data &_oup):
    tau_id(_tau_id), inputs(_inputs), oup(_oup) {
}
std::string IncreExampleData::toString() const {
    std::string res = "(" + std::to_string(tau_id) + ") {";
    bool flag = false;
    for (const auto& [name, t]: inputs) {
        if (flag) res += ","; res += (name + ": " + t.toString()); flag = true;
    }
    return res + "} -> " + oup.toString();
}
void IncreExamplePool::add(const IncreExample &example) {
    int id = example->tau_id;
    // LOG(INFO) << "Add " << example->toString();
    while (example_pool.size() <= id) example_pool.emplace_back();
    example_pool[id].push_back(example);
}

namespace {

    std::pair<Term, bool> _collectSubst(const Term& x, const std::string& name, const Term& y);

#define SubstHead(ty) std::pair<Term, bool> _collectSubst(Tm ## ty *x, const Term& _x, const std::string& name, const Term& y)
#define SubstCase(ty) return _collectSubst(dynamic_cast<Tm ## ty*>(x.get()), x, name, y)
#define SubstRes(field) auto [field ## _res, field ##_flag] = _collectSubst(x->field, name, y)
#define SubstRes2(field) auto [field ## _res, field ## _flag] = _collectSubst(field, name, y)

    SubstHead(Var) {
        if (x->name == name) return {y, true};
        return {_x, false};
    }
    SubstHead(Value) {
        return {_x, false};
    }
    SubstHead(Match) {
        SubstRes(def);
        bool flag = def_flag;
        std::vector<std::pair<Pattern, Term>> cases;
        for (const auto& [pt, branch]: x->cases) {
            if (incre::isUsed(pt, name)) {
                cases.emplace_back(pt, branch);
            } else {
                SubstRes2(branch);
                flag |= branch_flag;
                cases.emplace_back(pt, branch_res);
            }
        }
        if (!flag) return {_x, false};
        return {std::make_shared<TmMatch>(def_res, cases), true};
    }
    SubstHead(Tuple) {
        bool flag = false; TermList fields;
        for (const auto& field: x->fields) {
            SubstRes2(field);
            flag |= field_flag; fields.push_back(field_res);
        }
        if (!flag) return {_x, false};
        return {std::make_shared<TmTuple>(fields), true};
    }
    SubstHead(LabeledCreate) {
        SubstRes(def);
        if (!def_flag) return {_x, false};
        return {std::make_shared<TmLabeledCreate>(def_res, x->id), true};
    }
    SubstHead(Abs) {
        if (x->name == name) return {_x, false};
        SubstRes(content);
        if (!content_flag) return {_x, false};
        return {std::make_shared<TmAbs>(x->name, x->type, content_res), true};
    }
    SubstHead(App) {
        SubstRes(func); SubstRes(param);
        if (!func_flag && !param_flag) return {_x, false};
        return {std::make_shared<TmApp>(func_res, param_res), true};
    }
    SubstHead(Fix) {
        SubstRes(content);
        if (!content_flag) return {_x, false};
        return {std::make_shared<TmFix>(content_res), true};
    }
    SubstHead(If) {
        SubstRes(c); SubstRes(t); SubstRes(f);
        if (!c_flag && !t_flag && !f_flag) return {_x, false};
        return {std::make_shared<TmIf>(c_res, t_res, f_res), true};
    }
    SubstHead(Let) {
        SubstRes(def);
        if (x->name == name) {
            if (!def_flag) return {_x, false};
            return {std::make_shared<TmLet>(x->name, def_res, x->content), true};
        }
        SubstRes(content);
        if (!def_flag && !content_flag) return {_x, false};
        return {std::make_shared<TmLet>(x->name, def_res, content_res), true};
    }
    SubstHead(Proj) {
        SubstRes(content);
        if (!content_flag) return {_x, false};
        return {std::make_shared<TmProj>(content_res, x->id), true};
    }
    SubstHead(LabeledPass) {
        TermList defs; bool flag = false;
        for (const auto& def: x->defs) {
            SubstRes2(def); flag |= def_flag;
            defs.push_back(def_res);
        }
        for (auto& p_name: x->names) {
            if (p_name == name) {
                if (!flag) return {_x, false};
                return {std::make_shared<TmLabeledPass>(x->names, defs, x->content, x->tau_id, x->subst_info), true};
            }
        }
        SubstRes(content);
        if (!flag && !content_flag) return {_x, false};
        auto res = std::make_shared<TmLabeledPass>(x->names, defs, content_res, x->tau_id, x->subst_info);
        if (content_flag) res->addSubst(name, incre::run(y, nullptr));
        return {res, true};
    }

    std::pair<Term, bool> _collectSubst(const Term& x, const std::string& name, const Term& y) {
        switch (x->getType()) {
            case TermType::VAR: SubstCase(Var);
            case TermType::MATCH: SubstCase(Match);
            case TermType::VALUE: SubstCase(Value);
            case TermType::IF: SubstCase(If);
            case TermType::TUPLE: SubstCase(Tuple);
            case TermType::ABS: SubstCase(Abs);
            case TermType::APP: SubstCase(App);
            case TermType::LET: SubstCase(Let);
            case TermType::PROJ: SubstCase(Proj);
            case TermType::CREATE: SubstCase(LabeledCreate);
            case TermType::PASS: SubstCase(LabeledPass);
            case TermType::FIX: SubstCase(Fix);
        }
    }

    Term collectSubst(const Term& x, const std::string& name, const Term& y) {
        //LOG(INFO) << "collect subst " << x->toString() << " " << name;
        //LOG(INFO) << "subst " << y->toString();
        return _collectSubst(x, name, y).first;
    }

    Data _collectExample(const Term &term, Context *ctx, IncreExamplePool* pool);

#define CollectHead(name) Data _collectExample(Tm ## name* term, const Term& _term, Context* ctx, IncreExamplePool* pool)
#define CollectCase(name) return _collectExample(dynamic_cast<Tm ## name*>(term.get()), term, ctx, pool)
#define CollectSub(name) auto name = _collectExample(term->name, ctx, pool)
#define CollectSub2(name) auto name ## _res = _collectExample(name, ctx, pool)

    CollectHead(Value) {
        return term->data;
    }
    CollectHead(Proj) {
        CollectSub(content);
        auto* tv = dynamic_cast<VTuple*>(content.get());
        assert(tv);
        return tv->elements[term->id - 1];
    }
    CollectHead(Let) {
        CollectSub(def);
        auto content = collectSubst(term->content, term->name, std::make_shared<TmValue>(def));
        CollectSub2(content);
        return content_res;
    }
    CollectHead(Abs) {
        auto name = term->name;
        auto content = term->content;
        auto f = [name, content, ctx, pool](const Term& param) {
            auto c = collectSubst(content, name, param);
            CollectSub2(c);
            return c_res;
        };
        return Data(std::make_shared<VFunction>(f));
    }
    CollectHead(App) {
        CollectSub(func); CollectSub(param);
        auto* av = dynamic_cast<VFunction*>(func.get());
        assert(av);
        return av->func(std::make_shared<TmValue>(param));
    }
    CollectHead(Fix) {
        CollectSub(content);
        auto* av = dynamic_cast<VFunction*>(content.get());
        assert(av);
        return av->func(_term);
    }
    CollectHead(Match) {
        CollectSub(def);
        for (auto& [pt, branch]: term->cases) {
            if (isMatch(def, pt)) {
                auto binds = bindPattern(def, pt);
                auto res = branch;
                for (int i = int(binds.size()) - 1; i >= 0; --i) {
                    auto& [name, bind] = binds[i];
                    res = collectSubst(res, name, bind);
                }
                CollectSub2(res);
                return res_res;
            }
        }
        assert(0);
    }
    CollectHead(Tuple) {
        DataList fields;
        for (auto& field: term->fields) {
            CollectSub2(field);
            fields.push_back(field_res);
        }
        return Data(std::make_shared<VTuple>(fields));
    }
    CollectHead(Var) {
        auto def = ctx->getTerm(term->name);
        CollectSub2(def);
        return def_res;
    }
    CollectHead(If) {
        CollectSub(c);
        auto* bv = dynamic_cast<VBool*>(c.get());
        if (bv->w) {
            CollectSub(t); return t;
        } else {
            CollectSub(f); return f;
        }
    }
    CollectHead(LabeledCreate) {
        CollectSub(def);
        return Data(std::make_shared<VLabeledCompress>(def, term->id));
    }
    CollectHead(LabeledPass) {
        std::unordered_map<std::string, Data> inp = term->subst_info;
        Term content = term->content;
        for (int i = int(term->names.size()) - 1; i >= 0; --i) {
            auto def = term->defs[i];
            CollectSub2(def);
            auto* cv = dynamic_cast<VCompress*>(def_res.get());
            assert(cv);
            if (inp.find(term->names[i]) == inp.end()) {
                inp[term->names[i]] = def_res;
                content = collectSubst(content, term->names[i], std::make_shared<TmValue>(cv->content));
            }
        }
        CollectSub2(content);
        auto example = std::make_shared<IncreExampleData>(term->tau_id, inp, content_res);
        pool->add(example);
        return content_res;
    }

    Data _collectExample(const Term &term, Context *ctx, IncreExamplePool* pool) {
        //std::cout << std::endl;
        //LOG(INFO) << "Collect " << term->toString();
        switch (term->getType()) {
            case TermType::VALUE: CollectCase(Value);
            case TermType::PROJ: CollectCase(Proj);
            case TermType::LET: CollectCase(Let);
            case TermType::ABS: CollectCase(Abs);
            case TermType::APP: CollectCase(App);
            case TermType::CREATE: CollectCase(LabeledCreate);
            case TermType::PASS: CollectCase(LabeledPass);
            case TermType::FIX: CollectCase(Fix);
            case TermType::MATCH: CollectCase(Match);
            case TermType::TUPLE: CollectCase(Tuple);
            case TermType::VAR: CollectCase(Var);
            case TermType::IF: CollectCase(If);
        }
    }
}

void IncreExamplePool::generateExample() {
    auto start = generator->getStartTerm();
    _collectExample(start, ctx, this);
}
IncreExamplePool::~IncreExamplePool() {
    delete generator;
}
IncreExamplePool::IncreExamplePool(Context *_ctx, StartTermGenerator *_generator):
    ctx(_ctx), generator(_generator) {
}

namespace {
    void _buildCollectContext(const Command& command, Context* ctx, IncreExamplePool* pool) {
        switch (command->getType()) {
            case CommandType::DEF_IND: {
                incre::run(command, ctx); return;
            }
            case CommandType::BIND: {
                auto* cb = dynamic_cast<CommandBind*>(command.get());
                switch (cb->binding->getType()) {
                    case BindingType::TYPE: {
                        incre::run(command, ctx);
                        return;
                    }
                    case BindingType::TERM: {
                        auto *term_bind = dynamic_cast<TermBinding *>(cb->binding.get());
                        auto ty = incre::getType(term_bind->term, ctx);
                        Data res = _collectExample(term_bind->term, ctx, pool);
                        ctx->addBinding(cb->name, std::make_shared<TmValue>(res), ty);
                        return;
                    }
                }
            }
            case CommandType::IMPORT: {
                auto* ci = dynamic_cast<CommandImport*>(command.get());
                for (auto& c: ci->commands) _buildCollectContext(c, ctx, pool);
                return;
            }
        }
    }
}

Context * incre::buildCollectContext(const IncreProgram &program, IncreExamplePool* pool) {
    auto* ctx = new Context();
    for (const auto& command: program->commands) {
        _buildCollectContext(command, ctx, pool);
    }
    return ctx;
}