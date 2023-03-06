//
// Created by pro on 2022/9/26.
//

#include "istool/incre/autolifter/incre_plp.h"
#include "glog/logging.h"
#include "istool/solver/enum/enum_util.h"
#include "istool/incre/trans/incre_trans.h"

using namespace incre::autolifter;


std::string FExampleSpace::example2String(const IOExample &example) {
    std::string res = "{";
    for (int i = 0; i < value_list.size(); ++i) {
        if (i) res += ", ";
        res += value_list[i].first + ": " + example.first[i].toString();
    }
    res += "} -> " + example.second.toString();
    return res;
}
std::string FExampleSpace::example2String(int id) {
    return example2String(example_list[id]);
}
void FExampleSpace::addExample() {
    auto example = pool->example_pool[tau_id][example_list.size()];

    DataList inp;
    for (auto& [var_name, _]: value_list) {
        inp.push_back(example->inputs[var_name]);
    }
    example_list.emplace_back(inp, example->oup);
}
int FExampleSpace::acquireExample(int target_num, TimeGuard *guard) {
    while (pool->example_pool[tau_id].size() < target_num && guard->getRemainTime() > 0) {
        pool->generateExample();
    }
    target_num = std::min(target_num, int(pool->example_pool[tau_id].size()));
    while (example_list.size() < target_num) {
        addExample();
    }
    return target_num;
}
FExampleSpace::FExampleSpace(IncreExamplePool *_pool, int _tau_id, const PEnv& _env, AlignTypeInfoData* info):
        pool(_pool), tau_id(_tau_id), env(_env.get()) {

    for (auto& [var_name, var_type]: info->inp_types) {
        value_list.emplace_back(var_name, incre::typeFromIncre(var_type));
    }
}

Data FExampleSpace::runAux(int example_id, const AuxProgram &aux) {
    auto compress = env->run(aux.first.second.get(), example_list[example_id].first);
    if (aux.second.second) {
        auto* tv = dynamic_cast<VLabeledCompress*>(compress.get());
#ifdef DEBUG
        auto* mid_type = dynamic_cast<TLabeledCompress*>(aux.first.first.get());
        assert(tv && mid_type && tv->id == mid_type->id);
#endif
        return env->run(aux.second.second.get(), {tv->content});
    }
    return compress;
}

PLPTask::PLPTask(FExampleSpace *_example_space, const std::vector<GrammarEnumerateTool *> &_aux_grammar_list,
                 const std::vector<TypedProgramList> &_pre_res, GrammarEnumerateTool *_compress_grammar, const TypedProgram &_target,
                 const std::vector<int> &_path): example_space(_example_space), aux_grammar_list(_aux_grammar_list),
                 pre_res_list(_pre_res), compress_grammar(_compress_grammar), target(_target), path(_path) {
}

// TODO: add cache
Data PLPTask::runOup(int example_id) {
    return example_space->runOup(example_id, target.second.get(), path);
}
Data PLPTask::runInp(int example_id, const AuxProgram& program) {
    return example_space->runAux(example_id, program);
}

IOExample PLPTask::getIO(int example_id, const std::vector<AuxProgram> &aux_list) {
    DataList inp;
    for (auto& aux_program: aux_list) {
        inp.push_back(runInp(example_id, aux_program));
    }
    return {inp, runOup(example_id)};
}

int PLPTask::acquireExample(int target_num, int timeout) {
    auto* guard = new TimeGuard(timeout);
    auto res = example_space->acquireExample(target_num, guard);
    delete guard;
    return res;
}

#define ValueHead(name) auto* v = dynamic_cast<V ## name*>(data.get()); if (v)

Data incre::autolifter::eliminateCompress(const Data &data) {
    {ValueHead(Int) return data;}
    {ValueHead(Bool) return data;}
    {ValueHead(Compress) return v->content;}
    {
        ValueHead(Tuple) {
            DataList elements;
            for (int i = 0; i < v->elements.size(); ++i) {
                elements.push_back(eliminateCompress(v->elements[i]));
            }
            return Data(std::make_shared<VTuple>(elements));
        }
    }
    {
        ValueHead(Inductive) {
            return Data(std::make_shared<VInductive>(v->name, eliminateCompress(v->content)));
        }
    }
    LOG(FATAL) << "Unknown data " << data.toString();
}

Data incre::autolifter::openLabeledCompress(const Data &data, int label) {
    auto* v = dynamic_cast<VLabeledCompress*>(data.get());
    if (!v || v->id != label) LOG(FATAL) << "Unmatched compress id: get " << data.toString() << " but except " << label;
    return eliminateCompress(data);
}

std::string incre::autolifter::aux2String(const AuxProgram &program) {
    if (!program.second.first) {
        return program.first.first->getName() + "@" + program.first.second->toString();
    } else {
        return /*program.first.first->getName() + "@" +*/ program.first.second->toString() + " -> " + program.second.second->toString();
    }
}
namespace {
    TypedProgram _extractTypedProgram(const PProgram& program) {
        auto* ts = dynamic_cast<TypeLabeledDirectSemantics*>(program->semantics.get());
        assert(ts && ts->type);
        return {ts->type, program->sub_list[0]};
    }
}

TypeLabeledDirectSemantics::TypeLabeledDirectSemantics(const PType &_type):
        NormalSemantics("@" + _type->getName(), _type, (TypeList){_type}), type(_type) {
}
Data TypeLabeledDirectSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    return inp_list[0];
}

void GrammarEnumerateTool::extend() {
    int target_size = program_pool.size();
    auto dummy_info = std::make_shared<SynthInfo>("", TypeList(), PType(), grammar);
    std::vector<FunctionContext> collect_list; EnumConfig c(nullptr, nullptr, nullptr);
    solver::collectAccordingSize({dummy_info}, target_size + 1, collect_list, c);
    TypedProgramList res_list;
    for (auto& res: collect_list) {
        auto p = _extractTypedProgram(res[""]);
        if (p.second->size() == target_size) {
            res_list.push_back(p);
        }
    }
    program_pool.emplace_back(res_list);
}
TypedProgramList* GrammarEnumerateTool::acquirePrograms(int target_size) {
    if (target_size > size_limit) return nullptr;
    while (target_size >= program_pool.size()) extend();
    return &program_pool[target_size];
}
GrammarEnumerateTool::~GrammarEnumerateTool() {
    delete grammar;
}