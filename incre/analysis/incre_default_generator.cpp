//
// Created by pro on 2022/9/24.
//

#include "istool/incre/analysis/incre_instru_runtime.h"
#include "glog/logging.h"
#include <stack>

using namespace incre;

namespace {
    const int KSizeLimit = 10;
    const int KIntMin = -5;
    const int KIntMax = 5;

    // TODO: Deal with nested inductive type

    int _getSize(TyData* ty) {
        switch (ty->getType()) {
            case TyType::VAR:
            case TyType::IND: return 1;
            case TyType::ARROW: assert(0);
            case TyType::INT:
            case TyType::BOOL:
            case TyType::UNIT: return 0;
            case TyType::COMPRESS: {
                auto* tc = dynamic_cast<TyCompress*>(ty);
                return _getSize(tc->content.get());
            }
            case TyType::TUPLE: {
                auto* tt = dynamic_cast<TyTuple*>(ty);
                int res = 0;
                for (const auto& field: tt->fields) res += _getSize(field.get());
                return res;
            }
        }
    }
    struct RandomContext {
        std::minstd_rand* e;
        std::stack<int> size;
        std::vector<std::pair<std::string, TyInductive*>> ctx;
        RandomContext(std::minstd_rand* _e): e(_e) {}
        TyInductive* lookup(const std::string& name) {
            for (int i = int(ctx.size()); i; --i) {
                auto& [tname, type] = ctx[i - 1];
                if (tname == name) return type;
            }
            assert(0);
        }
        void distributeSize(int num) {
            assert(!size.empty());
            int current = size.top(); size.pop();
            if (current == 0) return;
            assert(current);
            std::vector<int> sub(num, 0);
            auto d = std::uniform_int_distribution<int>(0, num - 1);
            for (int i = 1; i < current; ++i) sub[d(*e)]++;
            for (auto w: sub) size.push(w);
        }
    };

    Data getRandomData(TyData* type, RandomContext* ctx);

#define RandomCase(name) return _getRandomData(dynamic_cast<Ty ## name*>(type), ctx)
#define RandomHead(name) Data _getRandomData(Ty ## name* type, RandomContext* ctx)

    RandomHead(Int) {
        auto d = std::uniform_int_distribution<int>(KIntMin, KIntMax);
        return BuildData(Int, d(*(ctx->e)));
    }
    RandomHead(Bool) {
        auto d = std::bernoulli_distribution();
        return BuildData(Bool, d(*(ctx->e)));
    }
    RandomHead(Unit) {
        return {};
    }
    RandomHead(Tuple) {
        DataList fields;
        for (auto& sub: type->fields) fields.push_back(getRandomData(sub.get(), ctx));
        return BuildData(Product, fields);
    }
    RandomHead(Var) {
        auto* ti = ctx->lookup(type->name);
        return getRandomData(ti, ctx);
    }
    RandomHead(Inductive) {
        ctx->ctx.emplace_back(type->name, type);
        std::vector<int> valid_indices;
        int size = ctx->size.top();
        for (int i = 0; i < type->constructors.size(); ++i) {
            auto [cname, ctype] = type->constructors[i];
            int current = _getSize(ctype.get());
            if (current == 0 && size) continue;
            if (current <= size) valid_indices.push_back(i);
        }
        assert(!valid_indices.empty());
        std::shuffle(valid_indices.begin(), valid_indices.end(), *(ctx->e));
        int pos = valid_indices[0];
        auto [cname, ctype] = type->constructors[pos];
        ctx->distributeSize(_getSize(ctype.get()));
        auto content = getRandomData(ctype.get(), ctx);
        ctx->ctx.pop_back();
        return Data(std::make_shared<VInductive>(cname, content));
    }

    Data getRandomData(TyData* type, RandomContext* ctx) {
        switch (type->getType()) {
            case TyType::INT: RandomCase(Int);
            case TyType::BOOL: RandomCase(Bool);
            case TyType::UNIT: RandomCase(Unit);
            case TyType::COMPRESS:
            case TyType::ARROW:
                assert(0);
            case TyType::TUPLE: RandomCase(Tuple);
            case TyType::VAR: RandomCase(Var);
            case TyType::IND: RandomCase(Inductive);
        }
    }
}

Term DefaultStartTermGenerator::getStartTerm() {
    Term res = std::make_shared<TmVar>(func_name);
    auto* ctx = new RandomContext(&random_engine);
    for (auto& type: inp_list) {
        assert(ctx->size.empty());
        int num = _getSize(type.get());
        if (num) {
            assert(num <= KSizeLimit);
            auto d = std::uniform_int_distribution<int>(num, KSizeLimit);
            int start_size = d(random_engine);
            ctx->size.push(start_size + 1);
            ctx->distributeSize(num);
        }
        auto current = std::make_shared<TmValue>(getRandomData(type.get(), ctx));
        res = std::make_shared<TmApp>(res, current);
    }
    LOG(INFO) << "start " << res->toString();
    return res;
}
DefaultStartTermGenerator::DefaultStartTermGenerator(const std::string& _func_name, TyData* type): func_name(_func_name), random_engine(123) {
    while (1) {
        auto* at = dynamic_cast<TyArrow*>(type);
        if (!at) return;
        inp_list.push_back(at->source);
        type = at->target.get();
    }
}