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

incre::example::DpExampleCollectionEvaluator::DpExampleCollectionEvaluator(IncreExampleCollector *_collector):
    collector(_collector) {
}

#define Eval(name, ctx_name) evaluate(term->name.get(), ctx_name)
#define EvalAssign(name, ctx_name) auto name = Eval(name, ctx_name)

#define Print(x) std::cout << #x " = " << x << std::endl;

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmValue *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmApp *term, const IncreContext &ctx) {
    // std::cout << "zyw: DpExampleCollectionEvaluator::_evaluate" << std::endl;
    // std::cout << "term = " << term->toString() << std::endl;

    // evaluate result
    EvalAssign(func, ctx); EvalAssign(param, ctx);
    auto* vc = dynamic_cast<VClosure*>(func.get());
    if (!vc) throw IncreSemanticsError("the evaluation result of TmApp func should be a closure, but got " + func.toString());
    auto new_ctx = vc->context.insert(vc->name, param);
    auto result = evaluate(vc->body.get(), new_ctx);

    if (term->func->getType() == TermType::APP) {
        auto sub_term = std::static_pointer_cast<syntax::TmApp>(term->func);
        if (!sub_term) {
            LOG(FATAL) << "sub_term is not TmApp";
        }

        // collect from sinsert
        // e.g. APP (APP sinsert (Nil unit)) sempty
        // input is param (evaluation result of term->param), output is def_3 (evaluation of def used in TmMatch)
        if (sub_term->func->toString() == "sinsert") {
            // std::cout << "in sinsert: " << std::endl;
            Data input_null = Data();
            input_null.value = std::make_shared<NullValue>();
            auto sub_param = evaluate(sub_term->param.get(), ctx);
            DpSolution new_sol = std::make_shared<DpSolutionData>(sub_param);
            collector->dp_example_pool.push_back(new_sol);
            
            // std::cout << "inp = null" << std::endl;
            // std::cout << "oup = " << new_sol->toString() << std::endl;

            // Print(input_null.toString());
            // Print(sub_param.toString());
        }

        // collect from sstep1
        // e.g. APP (APP sstep (func)) (Nil unit)
        // sstep1 f sol = concat (map f sol)
        // input is param (evaluation result of term->param), output is result (evaluation of term)
        if (sub_term->func->toString() == "sstep1") {
            // std::cout << "in sstep1: " << std::endl;

            // Print(func.toString());
            // Print(param.toString());
            // Print(vc->name);
            // Print(vc->body->toString());
            // Print(termType2String(vc->body->getType()));

            auto term_2 = std::static_pointer_cast<syntax::TmApp>(vc->body);
            auto func_2 = evaluate(term_2->func.get(), new_ctx);
            auto param_2 = evaluate(term_2->param.get(), new_ctx);
            auto* vc_2 = dynamic_cast<VClosure*>(func_2.get());
            if (!vc_2) throw IncreSemanticsError("the evaluation result of TmApp func should be a closure, but got " + func_2.toString());
            auto new_ctx_2 = vc_2->context.insert(vc_2->name, param_2);
            auto result_2 = evaluate(vc_2->body.get(), new_ctx_2);

            // Print(term_2->func->toString());
            // Print(term_2->param->toString());
            // Print(termType2String(vc_2->body->getType()));
            // Print(result_2.toString());

            auto term_3 = std::static_pointer_cast<syntax::TmMatch>(vc_2->body);
            auto def_3 = evaluate(term_3->def.get(), new_ctx_2);
            Data result_3;
            bool has_result_3 = false;
            for (auto& [pt, tm]: term_3->cases) {
                if (isValueMatchPattern(pt.get(), def_3)) {
                    auto new_ctx_3 = bindValueWithPattern(pt.get(), def_3, new_ctx_2);
                    result_3 = evaluate(tm.get(), new_ctx_3);
                    has_result_3 = true;
                    break;
                }
            }
            if (!has_result_3) {
                throw IncreSemanticsError("cannot match " + def_3.toString() + " with " + term_3->toString());
            }

            // Print(term_3->toString());
            // Print(def_3.toString());
            // Print(result_3.toString());
        
            // std::cout << "input = " << param.toString() << std::endl;
            // std::cout << "output = " << def_3.toString() << std::endl;

            // get input and output from Data
            DataList inp = ::data::data2DataList(param);
            DataList oup_tmp = ::data::data2DataList(def_3);
            std::vector<DataList> oup;
            for (auto& tmp: oup_tmp) {
                oup.push_back(::data::data2DataList(tmp));
            }
            if (inp.size() != oup.size()) {
                LOG(FATAL) << "size of input and output not match";
            }
            // add example into collector
            int inp_len = inp.size();
            std::vector<DpSolution> inp_sol_list;
            for (int i = 0; i < inp_len; ++i) {
                auto& inp_data = inp[i];
                DpSolution inp_sol = std::make_shared<DpSolutionData>(inp_data);
                inp_sol_list.push_back(inp_sol);
                collector->dp_example_pool.push_back(inp_sol);
                for (auto& oup_data: oup[i]) {
                    DpSolution oup_sol = std::make_shared<DpSolutionData>(oup_data);
                    inp_sol->children.push_back(oup_sol);
                    collector->dp_example_pool.push_back(oup_sol);
                }
            }

            for (int i = 0; i < inp_len; ++i) {
                for (int j = 0; j < inp_len; ++j) {
                    collector->dp_same_time_solution.push_back(std::make_pair(inp_sol_list[i], inp_sol_list[j]));
                    collector->dp_same_time_solution.push_back(std::make_pair(inp_sol_list[j], inp_sol_list[i]));
                }
            }
            
            // // print input and output
            // std::cout << "inp = " << std::endl;
            // for (auto& data: inp) {
            //     std::cout << data.toString() << std::endl;
            // }
            // std::cout << "oup = " << std::endl;
            // for (auto& data_list: oup) {
            //     for (auto& data: data_list) {
            //         std::cout << data.toString() << std::endl;
            //     }
            //     std::cout << std::endl;
            // }
        }
    }

    return result;
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmCons *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmFunc *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmIf *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmLabel *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmUnlabel *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmRewrite *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmLet *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmPrimary *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmMatch *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmVar *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmTuple *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

Data DpExampleCollectionEvaluator::_evaluate(syntax::TmProj *term, const IncreContext &ctx) {
    return DefaultEvaluator::_evaluate(term, ctx);
}

std::string DpSolutionData::toString() {
    return partial_solution.toString();
}

void DpSolutionSet::add(DpExample& example) {
    Data inp = example->inp;
    Data oup = example->oup;

    // if input is "null", don't add into solution_list
    if (inp.toString() == "null") {
        if (existing_sol.find(oup.toString()) == existing_sol.end()) {
            existing_sol.insert(oup.toString());
            sol_list.push_back(std::make_shared<DpSolutionData>(oup));
        }
        return;
    }

    if (existing_sol.find(inp.toString()) == existing_sol.end()) {
        existing_sol.insert(inp.toString());
        sol_list.push_back(std::make_shared<DpSolutionData>(inp));
    }
    if (existing_sol.find(oup.toString()) == existing_sol.end()) {
        existing_sol.insert(oup.toString());
        sol_list.push_back(std::make_shared<DpSolutionData>(oup));
    }
    // add transfer relation
    DpSolution input = find(inp);
    DpSolution output = find(oup);
    input->children.push_back(output);
}

DpSolution DpSolutionSet::find(Data data) {
    std::string data_str = data.toString();
    for (auto& sol: sol_list) {
        if (sol->toString() == data_str) {
            return sol;
        }
    }
    LOG(FATAL) << "solution not exist";
}

IncreExampleCollector::IncreExampleCollector(IncreProgramData *program,
                                             const std::vector<std::vector<std::string>> &_cared_vars,
                                             const std::vector<std::string> &_global_name):
                                             cared_vars(_cared_vars), global_name(_global_name), example_pool(_cared_vars.size()) {
    eval = new IncreExampleCollectionEvaluator(this);
    dp_eval = new DpExampleCollectionEvaluator(this);
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

void IncreExampleCollector::collectDp(const syntax::Term &start, const DataList &global) {
    std::unordered_map<std::string, Data> global_inp_map;
    for (int i = 0; i < global.size(); ++i) {
        global_inp_map[global_name[i]] = global[i];
    }
    ctx->setGlobalInput(global_inp_map); current_global = global;
    dp_eval->evaluate(start.get(), ctx->ctx);
}

IncreExampleCollector::~IncreExampleCollector() {
    delete eval;
}

void IncreExampleCollector::clear() {
    current_global.clear();
    for (auto& example_list: example_pool) example_list.clear();
    dp_example_pool.clear();
    dp_same_time_solution.clear();
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

// merge dp_example from IncreExampleCollector to IncreExamplePool
void IncreExamplePool::mergeDp(int main_id, IncreExampleCollector *collector, TimeGuard* guard) {
    // for (int example_id = 0; example_id < collector->dp_example_pool.size(); ++example_id) {
    //     auto& new_example = collector->dp_example_pool[example_id];
    //     auto feature = new_example->toString();
    //     // if this example is duplicate, don't add it
    //     if (existing_dp_example_set.find(feature) == existing_dp_example_set.end()) {
    //         existing_dp_example_set.insert(feature);
    //         dp_example_pool.push_back(new_example);
    //     }
    //     if ((example_id & 255) == 255 && guard && guard->getRemainTime() < 0) {
    //         break;
    //     }
    // }
    dp_example_pool = collector->dp_example_pool;
    dp_same_time_solution = collector->dp_same_time_solution;
    collector->clear();
}

std::pair<syntax::Term, DataList> IncreExamplePool::generateStart() {
    DataList global_inp;
    for (auto& ty: global_type_list) global_inp.push_back(generator->getRandomData(ty));
    std::uniform_int_distribution<int> start_dist(0, int(start_list.size()) - 1);
    auto& [start_name, params] = start_list[start_dist(generator->env->random_engine)];
    Term term = std::make_shared<TmVar>(start_name);
    for (auto& param_type: params) {
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
            // std::cout << "zyw: command declare, " << command->name << std::endl;
            auto* ci = dynamic_cast<CommandDeclare*>(command.get());
            global_type_list.push_back(ci->type);
            global_name_list.push_back(command->name);
        }
        if (command->isDecrorateWith(CommandDecorate::START)) {
            // std::cout << "zyw: command start, " << command->name << std::endl;
            auto param_list = _extractStartParamList(type_ctx->ctx.getFinalType(command->name, rewriter));
            start_list.emplace_back(command->name, param_list);
        }
    }
    if (start_list.empty()) {
        auto* start = program->commands[int(program->commands.size()) - 1].get();
        auto type = type_ctx->ctx.getFinalType(start->name, rewriter);
        start_list.emplace_back(start->name, _extractStartParamList(type));
    }

    // print start_list
    // std::cout << "zyw: start_list.size = " << start_list.size() << std::endl;
    // for (int i = 0; i < start_list.size(); ++i) {
    //     std::cout << start_list[i].first;
    //     for (int j = 0; j < start_list[i].second.size(); ++j) {
    //         std::cout << ", " << start_list[i].second[j]->toString();
    //     }
    //     std::cout << std::endl;
    // }
    delete rewriter;
}

IncreExamplePool::~IncreExamplePool() {
    delete generator;
}

void IncreExamplePool::generateSingleExample() {
    auto [term, global] = generateStart();
    auto* collector = new IncreExampleCollector(program.get(), cared_vars, global_name_list);

    // global::recorder.start("collect");
    collector->collect(term, global);
    // add single example into example_pool in merge function
    merge(0, collector, nullptr);
    // global::recorder.end("collect");
    delete collector;
}

syntax::Term IncreExamplePool::generateDpSingleExample() {
    auto [term, global] = generateStart();
    auto* collector = new IncreExampleCollector(program.get(), cared_vars, global_name_list);

    collector->collectDp(term, global);
    // add single example into example_pool in merge function
    mergeDp(0, collector, nullptr);
    delete collector;

    return term;
}

syntax::Term IncreExamplePool::generateSingleExample_2() {
    auto [term, global] = generateStart();
    return term;
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

void IncreExamplePool::changeKSizeLimit(int new_size_limit) {
    generator->KSizeLimit = new_size_limit;
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

void IncreExamplePool::clear() {
    dp_example_pool.clear();
    dp_same_time_solution.clear();
}