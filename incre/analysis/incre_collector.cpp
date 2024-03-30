//
// Created by pro on 2023/12/10.
//

#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/analysis/incre_instru_types.h"
#include "istool/basic/config.h"
#include "glog/logging.h"
#include <queue>
#include <thread>
#include <mutex>

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
    DataList local_inp;
    for (auto& name: collector->cared_vars[labeled_term->id]) {
        local_inp.push_back(ctx.getData(name));
    }
    collector->add(labeled_term->id, local_inp, oup);
    return oup;
}

IncreExampleCollector::IncreExampleCollector(IncreProgramData *program,
                                             const std::vector<std::vector<std::string>> &_cared_vars,
                                             const std::vector<std::string> &_global_name):
                                             cared_vars(_cared_vars), global_name(_global_name), example_pool(_cared_vars.size()) {
    eval = new IncreExampleCollectionEvaluator(this);
    ctx = buildContext(program, eval, nullptr);
}

void
IncreExampleCollector::add(int rewrite_id, const DataList &local_inp, const Data &oup) {
    auto example = std::make_shared<IncreExampleData>(rewrite_id, local_inp, current_global, oup);
    example_pool[rewrite_id].push_back(example);
}

void IncreExampleCollector::collect(const syntax::Term &start, const DataList &global) {
    std::unordered_map<std::string, Data> global_inp_map;
    for (int i = 0; i < global.size(); ++i) {
        global_inp_map[global_name[i]] = global[i];
    }
    ctx->setGlobalInput(global_inp_map); current_global = global;
    eval->evaluate(start.get(), ctx->ctx);
}

IncreExampleCollector::~IncreExampleCollector() {
    delete eval;
}

void IncreExampleCollector::clear() {
    current_global.clear();
    for (auto& example_list: example_pool) example_list.clear();
}

// merge collector->example_pool into IncreExamplePool->example_pool
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
            // if this example is unique, then add into example_pool
            if (existing_example_set[rewrite_id].find(feature) == existing_example_set[rewrite_id].end()) {
                existing_example_set[rewrite_id].insert(feature);
                example_pool[rewrite_id].push_back(new_example);
            }
            if ((example_id & 255) == 255 && guard && guard->getRemainTime() < 0) break;
        }
    }
    collector->clear();
}

std::pair<syntax::Term, DataList> IncreExamplePool::generateStart() {
    DataList global_inp;
    for (auto& ty: global_type_list) global_inp.push_back(generator->getRandomData(ty));
    std::uniform_int_distribution<int> start_dist(0, int(start_list.size()) - 1);
    auto& [start_name, params] = start_list[start_dist(generator->env->random_engine)];
    Term term = std::make_shared<TmVar>(start_name);
    // std::cout << "zyw: in generateStart()" << std::endl;
    for (auto& param_type: params) {
        // std::cout << param_type->toString() << std::endl;
        auto input_data = generator->getRandomData(param_type);
        term = std::make_shared<TmApp>(term, std::make_shared<TmValue>(input_data));
    }
    return {term, global_inp};
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
                                   program(_program), cared_vars(_cared_vars), generator(_g), is_finished(_cared_vars.size(), false),
                                   example_pool(cared_vars.size()), existing_example_set(cared_vars.size()){
    auto* env = generator->env;
    auto cv = env->getConstRef(config::KThreadNumName);
    thread_num = theory::clia::getIntValue(*cv);

    auto type_ctx = buildContext(_program.get(), [](){return nullptr;}, BuildGen(types::IncreLabeledTypeChecker));
    auto* rewriter = new IncreLabeledTypeRewriter();

    for (auto& command: program->commands) {
        if (command->getType() == CommandType::DECLARE && command->isDecrorateWith(CommandDecorate::INPUT)) {
            std::cout << "zyw: command declare, " << command->name << std::endl;
            auto* ci = dynamic_cast<CommandDeclare*>(command.get());
            global_type_list.push_back(ci->type);
            global_name_list.push_back(command->name);
        }
        if (command->isDecrorateWith(CommandDecorate::START)) {
            std::cout << "zyw: command start, " << command->name << std::endl;
            auto param_list = _extractStartParamList(type_ctx->ctx.getFinalType(command->name, rewriter));
            start_list.emplace_back(command->name, param_list);
        }
    }
    if (start_list.empty()) {
        auto* start = program->commands[int(program->commands.size()) - 1].get();
        auto type = type_ctx->ctx.getFinalType(start->name, rewriter);
        start_list.emplace_back(start->name, _extractStartParamList(type));
    }
    std::cout << "zyw: start_list.size = " << start_list.size() << std::endl;
    for (int i = 0; i < start_list.size(); ++i) {
        std::cout << start_list[i].first;
        for (int j = 0; j < start_list[i].second.size(); ++j) {
            std::cout << ", " << start_list[i].second[j]->toString();
        }
        std::cout << std::endl;
    }
    delete rewriter;
}

IncreExamplePool::~IncreExamplePool() {
    delete generator;
}

void IncreExamplePool::generateSingleExample() {
    auto [term, global] = generateStart();
    auto* collector = new IncreExampleCollector(program.get(), cared_vars, global_name_list);

    global::recorder.start("collect");
    collector->collect(term, global);
    // add single example into example_pool in merge function
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
    std::queue<std::pair<Term, DataList>> input_queue;

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
        auto *collector = new IncreExampleCollector(program.get(), cared_vars, global_name_list);
        collector_list.push_back(collector);
        thread_list.emplace_back(single_thread, collector, i);
    }
    for (int i = 0; i < thread_num; ++i) {
        thread_list[i].join();
        delete collector_list[i];
    }

    if (example_pool[rewrite_id].size() < target_num) is_finished[rewrite_id] = true;
}

void IncreExamplePool::printCaredVars() {
    int len1 = cared_vars.size();
    for (int i = 0; i < len1; ++i) {
        std::cout << i << ", cared_vars = ";
        int len2 = cared_vars[i].size();
        for (int j = 0; j < len2; ++j) {
            if (j) std::cout << ", ";
            std::cout << cared_vars[i][j];
        }
        std::cout << std::endl;
    }
}

void IncreExamplePool::printStartList() {
    int len1 = start_list.size();
    for (int i = 0; i < len1; ++i) {
        std::cout << i << ", first = " << start_list[i].first << ", second = ";
        int len2 = start_list[i].second.size();
        for (int j = 0; j < len2; ++j) {
            if (j) std::cout << ", ";
            std::cout << start_list[i].second[j]->toString();
        }
        std::cout << std::endl;
    }
}

void IncreExamplePool::printExistingExampleSet() {
    int len = existing_example_set.size();
    for (int i = 0; i < len; ++i) {
        std::cout << i;
        for (auto& j : existing_example_set[i]) {
            std::cout << ", " << j << std::endl;
        }
    }
}

void IncreExamplePool::printGlobalNameList() {
    int len = global_name_list.size();
    for (int i = 0; i < len; ++i) {
        if (i) std::cout << ", ";
        std::cout << global_name_list[i];
    }
    std::cout << std::endl;
}

void IncreExamplePool::printGlobalTypeList() {
    int len = global_type_list.size();
    for (int i = 0; i < len; ++i) {
        if (i) std::cout << ", ";
        std::cout << global_type_list[i]->toString();
    }
    std::cout << std::endl;
}