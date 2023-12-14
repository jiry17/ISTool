//
// Created by pro on 2023/12/7.
//

#ifndef ISTOOL_INCRE_TYPES_H
#define ISTOOL_INCRE_TYPES_H

#include "incre_syntax.h"
#include "incre_context.h"

namespace incre::types {
    class IncreTypingError: public std::exception {
    private:
        std::string message;
    public:
        IncreTypingError(const std::string _message);
        virtual const char* what() const noexcept;
    };

    syntax::Ty getSyntaxValueType(const Data& data);
    syntax::Ty getPrimaryType(const std::string& name);

#define RegisterAbstractTypingRule(name) virtual syntax::Ty _typing(syntax::Tm ## name* term, const IncreContext& ctx) = 0;
#define RegisterAbstractUnifyRule(name) virtual void _unify(syntax::Ty ## name* x, syntax::Ty ## name* y, const syntax::Ty& _x, const syntax::Ty& _y) = 0;
    class IncreTypeChecker {
    protected:
        int tmp_var_id = 0, _level = 0;
        syntax::Ty getTmpVar();
        void pushLevel(); void popLevel();
        TERM_CASE_ANALYSIS(RegisterAbstractTypingRule);
        TYPE_CASE_ANALYSIS(RegisterAbstractUnifyRule);
        virtual void preProcess(syntax::TermData* term, const IncreContext& ctx) = 0;
        virtual void postProcess(syntax::TermData* term, const IncreContext& ctx, const syntax::Ty& res) = 0;
        virtual syntax::Ty instantiate(const syntax::Ty& x) = 0;
        virtual syntax::Ty generalize(const syntax::Ty& x) = 0;

        void unify(const syntax::Ty &x, const syntax::Ty &y);
    public:
        syntax::Ty typing(syntax::TermData* term, const IncreContext& ctx);
        syntax::Ty bindTyping(syntax::TermData* term, const IncreContext& ctx);
        virtual ~IncreTypeChecker() = default;
    };

#define RegisterTypingRule(name) virtual syntax::Ty _typing(syntax::Tm ## name* term, const IncreContext& ctx);
#define RegisterUnifyRule(name) virtual void _unify(syntax::Ty ## name* x, syntax::Ty ## name* y, const syntax::Ty& _x, const syntax::Ty& _y);

    class DefaultIncreTypeChecker: public IncreTypeChecker {
    protected:
        TERM_CASE_ANALYSIS(RegisterTypingRule);
        TYPE_CASE_ANALYSIS(RegisterUnifyRule);
        virtual syntax::Ty instantiate(const syntax::Ty& x);
        virtual syntax::Ty generalize(const syntax::Ty& x);
        virtual void preProcess(syntax::TermData* term, const IncreContext& ctx);
        virtual void postProcess(syntax::TermData* term, const IncreContext& ctx, const syntax::Ty& res);
        virtual void updateLevelBeforeUnification(syntax::TypeData* x, int index, int level);
        virtual std::pair<syntax::Ty, IncreContext> processPattern(syntax::PatternData* pattern, const IncreContext& ctx);
    public:
        virtual ~DefaultIncreTypeChecker() = default;
    };

    typedef std::function<IncreTypeChecker*()> IncreTypeCheckerGenerator;
}

#endif //ISTOOL_INCRE_TYPES_H
