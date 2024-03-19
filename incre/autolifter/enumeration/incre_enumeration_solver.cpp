//
// Created by pro on 2024/3/19.
//

#include "istool/incre/autolifter/incre_enumeration_solver.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/invoker/invoker.h"
#include "istool/incre/autolifter/incre_solver_util.h"
#include "istool/basic/specification.h"
#include "glog/logging.h"

using namespace incre;

IncreEnumerationSolver::IncreEnumerationSolver(IncreInfo *_info, const PEnv& _env): info(_info), env(_env) {
}

namespace {
    Data _runCompress(const Data& data, const PProgram& compress, Env* env, const DataList& global_list) {
        auto* vc = dynamic_cast<VLabeledCompress*>(data.get());
        if (vc) {
            assert(vc->id == 0);
            DataList inp; inp.push_back(vc->content);
            for (auto& val: global_list) inp.push_back(val);
            return env->run(compress.get(), inp);
        }
        auto* ct = dynamic_cast<VTuple*>(data.get());
        if (ct) {
            DataList fields;
            for (auto& val: ct->elements) fields.push_back(_runCompress(val, compress, env, global_list));
            return BuildData(Product, fields);
        }
        auto* ci = dynamic_cast<VInductive*>(data.get());
        if (ci) {
            auto new_content = _runCompress(ci->content, compress, env, global_list);
            return Data(std::make_shared<VInductive>(ci->name, new_content));
        }
        return data;
    }
}

IncreSolution IncreEnumerationSolver::solve() {
    assert(incre::getCompressTypeList(info).size() == 1);

    std::string compress_name;
    for (auto& command: info->program->commands) {
        if (command->isDecoratedWith(CommandDecorate::AS_COMPRESS)) {
            auto* cb = dynamic_cast<CommandBind*>(command.get());
            assert(cb && cb->binding->getType() == BindingType::TERM);
            compress_name = cb->name;
        }
    }
    if (compress_name.empty()) {
        LOG(FATAL) << "IncreEnumationSolver requires the user to provide the compression function via @AsCompress";
    }

    PProgram compress; Ty res_type;
    {
        ProgramList inp_list;
        inp_list.push_back(program::buildParam(0, incre::typeFromIncre(getCompressTypeList(info)[0])));
        for (auto& [global_name, global_type]: info->example_pool->input_list) {
            int inp_id = inp_list.size();
            inp_list.push_back(program::buildParam(inp_id, incre::typeFromIncre(global_type)));
        }
        for (auto &align_component: info->component_pool.align_list) {
            if (align_component->name == compress_name) {
                auto *ic = dynamic_cast<grammar::IncreComponent *>(align_component.get());
                assert(ic);
                auto sem = std::make_shared<grammar::IncreOperatorSemantics>(ic->name, ic->data, ic->is_parallel);
                compress = std::make_shared<Program>(sem, inp_list);
                res_type = incre::typeToIncre(ic->res_type.get());
            }
        }
    }
    assert(compress && res_type);

    int KExampleNum = 1000, KTimeOut = 5;
    IncreSolution sol; sol.compress_type_list.push_back(res_type);

    for (int align_id = 0; align_id < info->align_infos.size(); ++align_id) {
        info->example_pool->generateBatchedExample(align_id, KExampleNum, new TimeGuard(KTimeOut * 1000.0));
        IOExampleList examples;
        auto& align_info = info->align_infos[align_id];

        std::vector<std::string> local_names; TypeList var_types;
        for (auto& [var_name, var_type]: align_info->inp_types) {
            auto new_var_type = incre::rewriteTypeWithTypes(var_type, {res_type});
            local_names.push_back(var_name);
            var_types.push_back(incre::typeFromIncre(new_var_type));
        }
        auto oup_type = incre::typeFromIncre(incre::rewriteTypeWithTypes(align_info->oup_type, {res_type}));
        for (auto& [_, var_type]: info->example_pool->input_list) var_types.push_back(incre::typeFromIncre(var_type));


        for (int i = 0; i < KExampleNum && i < info->example_pool->example_pool[align_id].size(); ++i) {
            DataList inp_list; auto& example = info->example_pool->example_pool[align_id][i];
            DataList global_list;
            for (auto& [var_name, _]: info->example_pool->input_list) {
                global_list.push_back(example->global_inputs[var_name]);
            }
            for (auto& local_name: local_names) {
                inp_list.push_back(_runCompress(example->local_inputs[local_name], compress, env.get(), global_list));
            }
            for (auto& v: global_list) inp_list.push_back(v);
            Data oup = _runCompress(example->oup, compress, env.get(), global_list);
            examples.emplace_back(inp_list, oup);
        }

        auto grammar = info->component_pool.buildCombinatorGrammar(var_types, oup_type, align_info->command_id);
        auto synthesis_info = std::make_shared<SynthInfo>("f", var_types, oup_type, grammar);
        auto example_space = example::buildFiniteIOExampleSpace(examples, "f", env.get(), var_types, oup_type);
        auto* spec = new Specification({synthesis_info}, env, example_space);
        InvokeConfig config; config.set("runnable", [](const Program&) -> bool {return false;});
        auto res = invoker::synthesis(spec, new FiniteExampleVerifier(example_space.get()), SolverToken::OBSERVATIONAL_EQUIVALENCE, nullptr, config);
        if (res.empty()) LOG(FATAL) << "synthesis failed";

        TermList term_list;
        for (auto& var_name: local_names) term_list.push_back(std::make_shared<TmVar>(var_name));
        for (auto& [global_name, _]: info->example_pool->input_list) term_list.push_back(std::make_shared<TmVar>(global_name));
        auto res_term = incre::autolifter::util::program2Term(res["f"].get(), info->component_pool.comb_list, term_list);
        sol.align_list.push_back(res_term);
    }
    return sol;
}