//
// Created by pro on 2023/12/10.
//

#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/analysis/incre_instru_types.h"
#include "istool/basic/config.h"
#include "glog/logging.h"
#include <queue>
#include <thread>

using namespace incre;
using namespace incre::syntax;
using namespace incre::semantics;
using namespace incre::example;

incre::example::IncreExampleCollectionEvaluator::IncreExampleCollectionEvaluator(IncreExampleCollector *_collector):
    collector(_collector) {
}

Data IncreExampleCollectionEvaluator::_evaluate(syntax::TmRewrite *term, const IncreContext &ctx) {
    auto* labeled_term = dynamic_cast<TmLabeledRewrite*>(term);
    if (!labeled_term) LOG(INFO) << "Expected TmLabeledRewrite, but got " << term->toString();
    auto oup = evaluate(labeled_term->body.get(), ctx);
    std::unordered_map<std::string, Data> local_inp_map;
    for (auto& name: collector->cared_vars[labeled_term->id]) {
        local_inp_map[name] = ctx.getData(name);
    }
    collector->add(labeled_term->id, local_inp_map, oup);
    return oup;
}

IncreExampleCollector::IncreExampleCollector(IncreProgramData *program,
                                             const std::vector<std::vector<std::string>> &_cared_vars):
                                             cared_vars(_cared_vars), example_pool(_cared_vars.size()) {
    eval = new IncreExampleCollectionEvaluator(this);
    ctx = buildContext(program, eval, nullptr, nullptr);
}

void
IncreExampleCollector::add(int rewrite_id, const std::unordered_map<std::string, Data> &local_inp, const Data &oup) {
    auto example = std::make_shared<IncreExampleData>(rewrite_id, local_inp, current_global, oup);
    example_pool[rewrite_id].push_back(example);
}

void IncreExampleCollector::collect(const syntax::Term &start, const std::unordered_map<std::string, Data> &global) {
    ctx->setGlobalInput(global); current_global = global;
    eval->evaluate(start.get(), ctx->ctx);
}

IncreExampleCollector::~IncreExampleCollector() {
    delete eval;
}

void IncreExampleCollector::clear() {
    current_global.clear();
    for (auto& example_list: example_pool) example_list.clear();
}

void IncreExamplePool::merge(int main_id, IncreExampleCollector *collector, TimeGuard* guard) {
    assert(collector->example_pool.size() == example_pool.size());
    std::vector<int> index_order;
    index_order.push_back(main_id);
    for (int i = 0; i < example_pool.size(); ++i) {
        if (i != main_id) index_order.push_back(i);
    }
    for (auto rewrite_id: index_order) {
        for (int example_id = 0; example_id < collector->example_pool[rewrite_id].size(); ++example_id) {
            auto& new_example = collector->example_pool[rewrite_id][example_id];
            auto feature = new_example->toString();
            if (existing_example_set[rewrite_id].find(feature) == existing_example_set[rewrite_id].end()) {
                existing_example_set[rewrite_id].insert(feature);
                example_pool[rewrite_id].push_back(new_example);
            }
            if ((example_id & 255) == 255 && guard && guard->getRemainTime() < 0) break;
        }
    }
    collector->clear();
}

std::pair<syntax::Term, std::unordered_map<std::string, Data>> IncreExamplePool::generateStart() {
    std::unordered_map<std::string, Data> global;
    for (auto& [inp_name, inp_ty]: global_input_list) {
        auto input_data = generator->getRandomData(inp_ty);
        global[inp_name] = input_data;
    }
    std::uniform_int_distribution<int> start_dist(0, int(start_list.size()) - 1);
    auto& [start_name, params] = start_list[start_dist(generator->env->random_engine)];
    Term term = std::make_shared<TmVar>(start_name);
    for (auto& param_type: params) {
        auto input_data = generator->getRandomData(param_type);
        term = std::make_shared<TmApp>(term, std::make_shared<TmValue>(input_data));
    }
    return {term, global};
}

namespace {
    TyList _extractStartParamList(const Ty& type) {
        if (type->getType() == TypeType::POLY) {
            LOG(FATAL) << "Start term should not be polymorphic, but get " << type->toString();
        }
        TyList params; Ty current(type);
        while (current->getType() == TypeType::ARR) {
            auto* ty_arr = dynamic_cast<TyArr*>(current.get());
            params.push_back(ty_arr->inp); current = ty_arr->oup;
        }
        return params;
    }
}

IncreExamplePool::IncreExamplePool(const IncreProgram &_program,
                                   const std::vector<std::vector<std::string>> &_cared_vars, IncreDataGenerator *_g):
                                   program(_program), cared_vars(_cared_vars), generator(_g),
                                   example_pool(cared_vars.size()), existing_example_set(cared_vars.size()){
    auto* env = generator->env;
    auto cv = env->getConstRef(config::KThreadNumName);
    thread_num = theory::clia::getIntValue(*cv);

    auto type_ctx = buildContext(_program.get(), [](){return nullptr;},
                                 BuildGen(types::IncreLabeledTypeChecker), BuildGen(syntax::IncreLabeledTypeRewriter));

    for (auto& command: program->commands) {
        if (command->getType() == CommandType::INPUT) {
            auto* ci = dynamic_cast<CommandInput*>(command.get());
            global_input_list.emplace_back(command->name, ci->type);
        }
        if (command->isDecrorateWith(CommandDecorate::START)) {
            auto param_list = _extractStartParamList(type_ctx->ctx.getType(command->name));
            start_list.emplace_back(command->name, param_list);
        }
    }
    if (start_list.empty()) {
        auto* start = type_ctx->ctx.start.get();
        if (!start) LOG(FATAL) << "Unexpected empty context";
        start_list.emplace_back(start->name, _extractStartParamList(start->bind.getType()));
    }
}

IncreExamplePool::~IncreExamplePool() {
    delete generator;
}

void IncreExamplePool::generateSingleExample() {
    auto [term, global] = generateStart();
    auto* collector = new IncreExampleCollector(program.get(), cared_vars);

    global::recorder.start("collect");
    collector->collect(term, global);
    merge(0, collector, nullptr);
    global::recorder.end("collect");
    delete collector;
}

namespace {
    const int KMaxFailedAttempt = 500;
}

void IncreExamplePool::generateBatchedExample(int rewrite_id, int target_num, TimeGuard *guard) {
    if (is_finished[rewrite_id] || target_num < example_pool[rewrite_id].size()) return;

    std::mutex input_lock, res_lock;
    bool is_all_finished = false;
    int attempt_num = 0;
    std::queue<std::pair<Term, std::unordered_map<std::string, Data>>> input_queue;

    auto single_thread = [&](IncreExampleCollector *collector, int id) {
        int previous_num = 0;
        while (!guard || guard->getRemainTime() > 0) {
            if (res_lock.try_lock()) {
                int pre_size = example_pool[rewrite_id].size();

                int total_size = 0;
                for (auto& example_list: collector->example_pool) {
                    total_size += example_list.size();
                }
                merge(rewrite_id, collector, guard);
                collector->clear();
                if (example_pool[rewrite_id].size() == pre_size) {
                    attempt_num += previous_num;
                    if (attempt_num >= KMaxFailedAttempt) {
                        is_all_finished = true;
                        is_finished[rewrite_id] = true;
                    }
                } else attempt_num = 0;
                if (example_pool[rewrite_id].size() >= target_num || is_all_finished) {
                    res_lock.unlock(); break;
                }
                res_lock.unlock();
            }

            input_lock.lock();
            if (input_queue.empty()) {
                int generate_num = thread_num * 100;
                for (int i = 0; i < generate_num; ++i) {
                    input_queue.push(generateStart());
                }
            }
            auto [start, global] = input_queue.front();
            input_queue.pop();
            input_lock.unlock();

            int pre_size = collector->example_pool[rewrite_id].size();
            collector->collect(start, global);
            if (collector->example_pool[rewrite_id].size() != pre_size)
                previous_num++;
        }
        return;
    };

    std::vector<std::thread> thread_list;
    std::vector<IncreExampleCollector*> collector_list;

    for (int i = 0; i < thread_num; ++i) {
        auto *collector = new IncreExampleCollector(program.get(), cared_vars);
        collector_list.push_back(collector);
        thread_list.emplace_back(single_thread, collector, i);
    }
    for (int i = 0; i < thread_num; ++i) {
        thread_list[i].join();
        delete collector_list[i];
    }

    if (example_pool[rewrite_id].size() < target_num) is_finished[rewrite_id] = true;
}