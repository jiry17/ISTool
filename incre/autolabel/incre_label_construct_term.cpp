//
// Created by pro on 2022/12/8.
//

#include "istool/incre/autolabel/incre_autolabel.h"
#include <unordered_set>
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolabel;

namespace {
    struct ConstructTermInfo {
        TermList unlabel_list;
        TermList pass_list;
        void print() const {
            std::cout << "unlabel terms" << std::endl;
            for (auto& term: unlabel_list) std::cout << "  " << term->toString() << std::endl;
            std::cout << "pass terms" << std::endl;
            for (auto& term: pass_list) std::cout << "  " << term->toString() << std::endl;
        }
    };

    ConstructTermInfo _merge(const ConstructTermInfo& x, const ConstructTermInfo& y) {
        ConstructTermInfo res = x;
        for (auto& term: y.unlabel_list) res.unlabel_list.push_back(term);
        for (auto& term: y.pass_list) res.pass_list.push_back(term);
        return res;
    }

    ConstructTermInfo _filter(const ConstructTermInfo& x, const std::vector<std::string>& names) {
        std::unordered_set<std::string> name_map;
        for (auto& name: names) name_map.insert(name);

        auto is_invalid = [&](const Term& term) {
            for (auto& free_name: getUnboundedVars(term.get())) {
                if (name_map.find(free_name) != name_map.end()) return false;
            }
            return true;
        };

        ConstructTermInfo res;
        for (const auto& term: x.unlabel_list) {
            if (is_invalid(term)) {
                LOG(FATAL) << "Term " << term->toString() << " is not covered by any pass";
            }
            res.unlabel_list.push_back(term);
        }
        for (const auto& term: x.pass_list) {
            if (!is_invalid(term)) res.pass_list.push_back(term);
        }
        return res;
    }

    typedef std::pair<Term, ConstructTermInfo> ConstructTermRes;

    std::vector<std::string> _getTmpNames(int num, const std::vector<std::string>& free_names) {
        std::unordered_set<std::string> free_set;
        for (auto& name: free_names) free_set.insert(name);
        std::vector<std::string> tmp_names;
        for (int id = 0;; ++id) {
            for (char ch = 'a'; ch <= 'z'; ++ch) {
                auto name = std::string(1, ch);
                if (id) name += std::to_string(id);
                if (free_set.find(name) == free_set.end()) {
                    tmp_names.push_back(name);
                    if (tmp_names.size() == num) return tmp_names;
                }
            }
        }
    }

    ConstructTermRes _buildPass(const ConstructTermRes & x) {
        if (x.second.unlabel_list.empty()) return x;
        int term_num = x.second.unlabel_list.size() + x.second.pass_list.size();
        std::vector<std::string> name_list = _getTmpNames(term_num, getUnboundedVars(x.first.get()));

        std::unordered_map<TermData*, Term> replace_map;
        int pre_size = x.second.unlabel_list.size();
        for (int i = 0; i < x.second.pass_list.size(); ++i) {
            replace_map[x.second.pass_list[i].get()] = std::make_shared<TmVar>(name_list[pre_size + i]);
        }

        auto replace_func = [&](const Term& term) -> Term {
            auto it = replace_map.find(term.get());
            if (it == replace_map.end()) return nullptr;
            return it->second;
        };
        TermList replaced_unlabeled_list;
        for (auto& term: x.second.unlabel_list) replaced_unlabeled_list.push_back(incre::replaceTerm(term, replace_func));

        for (int i = 0; i < x.second.unlabel_list.size(); ++i) {
            replace_map[x.second.unlabel_list[i].get()] = std::make_shared<TmVar>(name_list[i]);
        }
        auto base = incre::replaceTerm(x.first, replace_func);

        LOG(INFO) << "replace";
        for (auto& term: x.second.pass_list) std::cout << "  previous pass " << term->toString() << std::endl;
        std::cout << x.first->toString() << std::endl;
        std::cout << base->toString() << std::endl;
        {
            std::vector<std::string> names;
            for (int i = 0; i < replaced_unlabeled_list.size(); ++i) {
                names.push_back(name_list[i]);
            }
            base = std::make_shared<TmPass>(names, replaced_unlabeled_list, base);
        }
        for (int i = int(x.second.pass_list.size()) - 1; i >= 0; --i) {
            auto name = name_list[x.second.unlabel_list.size() + i];
            auto def = x.second.pass_list[i];
            base = std::make_shared<TmLet>(name, def, base);
        }

        ConstructTermInfo res_info;
        res_info.pass_list = x.second.pass_list;
        // if (x.second.pass_list.empty()) res_info.pass_list.push_back(base);

        return {base, res_info};
    }

#define ConstructTermHead(name) ConstructTermRes _constructTerm(Tm ## name* term, const Term& _term, LabelContext* ctx)
#define ConstructTermCase(name) {res = _constructTerm(dynamic_cast<Tm ## name*>(term.get()), term, ctx); break;}
#define ConstructSubTerm(name) auto [name, name##_info] = _constructTerm(term->name, ctx)

    ConstructTermRes _constructTerm(const Term& term, LabelContext* ctx);

    ConstructTermHead(If) {
        ConstructSubTerm(c); ConstructSubTerm(t); ConstructSubTerm(f);
        return {std::make_shared<TmIf>(c, t, f), _merge(_merge(c_info, t_info), f_info)};
    }
    ConstructTermHead(App) {
        ConstructSubTerm(func); ConstructSubTerm(param);
        return {std::make_shared<TmApp>(func, param), _merge(func_info, param_info)};
    }
    ConstructTermHead(Abs) {
        ConstructSubTerm(content);
        auto type = constructType(term->type, ctx);
        auto info = _filter(content_info, {term->name});
        return {std::make_shared<TmAbs>(term->name, type, content), info};
    }
    ConstructTermHead(Proj) {
        ConstructSubTerm(content);
        return {std::make_shared<TmProj>(content, term->id), content_info};
    }
    ConstructTermHead(Tuple) {
        TermList fields; ConstructTermInfo info;
        for (auto& field: term->fields) {
            auto [res, field_info] = _constructTerm(field, ctx);
            info = _merge(info, field_info);
            fields.push_back(res);
        }
        return {std::make_shared<TmTuple>(fields), info};
    }


    ConstructTermHead(Match) {
        ConstructSubTerm(def);
        auto res_info = def_info;
        std::vector<std::pair<Pattern, Term>> cases;
        for (auto& [pt, sub_term]: term->cases) {
            auto [sub_res, sub_info] = _constructTerm(sub_term, ctx);
            cases.emplace_back(pt, sub_res);
            res_info = _merge(res_info, _filter(sub_info, collectNames(pt.get())));
        }
        return {std::make_shared<TmMatch>(def, cases), res_info};
    }
    ConstructTermHead(Let) {
        ConstructSubTerm(def); ConstructSubTerm(content);
        auto info = _merge(def_info, _filter(content_info, {term->name}));
        return {std::make_shared<TmLet>(term->name, def, content), info};
    }
    ConstructTermHead(Fix) {
        ConstructSubTerm(content);
        return {std::make_shared<TmFix>(content), content_info};
    }

    ConstructTermRes _constructTerm(const Term& term, LabelContext* ctx) {
        ConstructTermRes res;
        switch (term->getType()) {
            case TermType::VAR:
            case TermType::VALUE: {
                res.first = term; break;
            }
            case TermType::IF: ConstructTermCase(If)
            case TermType::APP: ConstructTermCase(App)
            case TermType::ABS: ConstructTermCase(Abs)
            case TermType::PROJ: ConstructTermCase(Proj)
            case TermType::TUPLE: ConstructTermCase(Tuple)
            case TermType::CREATE:
            case TermType::PASS:
                LOG(FATAL) << "Unexpected term type CREATE and PASS";
            case TermType::MATCH: ConstructTermCase(Match)
            case TermType::LET: ConstructTermCase(Let)
            case TermType::FIX: ConstructTermCase(Fix)
        }
        {
            auto it = ctx->flip_map.find(term.get());
            if (it != ctx->flip_map.end()) {
                auto flip_label = ctx->getLabel(it->second);

                auto unlabel_it = ctx->unlabel_info_map.find(term.get());
                assert(unlabel_it != ctx->unlabel_info_map.end());
                auto unlabel_label = ctx->getLabel(unlabel_it->second);
                if (unlabel_label) {
                    res.second.unlabel_list.push_back(res.first);
                } else if (flip_label) {
                    res.first = std::make_shared<TmCreate>(res.first);
                }
            }
        }
        {
            auto it = ctx->possible_pass_map.find(term.get());
            if (it != ctx->possible_pass_map.end()) {
                auto pass_label = ctx->getLabel(it->second);
                if (pass_label) {
                    res = _buildPass(res);
                }
            }
        }
        LOG(INFO) << "label result " << term->toString();
        std::cout << "res: " << res.first->toString() << std::endl;
        res.second.print();
        return res;
    }
}

Term autolabel::constructTerm(const Term &term, LabelContext *ctx) {
    auto res = _constructTerm(term, ctx);
    if (!res.second.unlabel_list.empty()) {
        LOG(FATAL) << "Unlabel term " << res.second.unlabel_list[0]->toString() << " is not covered by pass";
    }
    return res.first;
}