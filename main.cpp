#include <iostream>

#include "z3++.h"
#include "istool/basic/config.h"
#include "istool/sygus/sygus.h"
#include "istool/solver/solver.h"
#include "istool/solver/component/component_based_solver.h"
#include "istool/solver/component/linear_encoder.h"
#include "istool/solver/enum/enum_solver.h"
#include "istool/solver/vsa/vsa_solver.h"
#include "istool/solver/stun/eusolver.h"
#include "istool/solver/polygen/lia_solver.h"
#include "istool/solver/polygen/polygen.h"
#include "istool/solver/polygen/polygen_term_solver.h"
#include "istool/solver/polygen/dnf_learner.h"
#include "istool/solver/polygen/polygen_cegis.h"
#include "istool/solver/maxflash/topdown_context_graph.h"
#include "istool/solver/maxflash/maxflash.h"
#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_witness.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/witness/theory_witness.h"
#include "istool/sygus/parser/json_util.h"
#include "istool/selector/split/z3_split_selector.h"
#include "glog/logging.h"

FunctionContext CBSInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);

    std::unordered_map<std::string, Z3GrammarEncoder*> encoder_map;
    for (const auto& info: spec->info_list) {
        encoder_map[info->name] = new LinearEncoder(info->grammar, spec->env.get());
    }
    auto* cbs = new ComponentBasedSynthesizer(spec, encoder_map);
    auto* solver = new CEGISSolver(cbs, v);

    auto res = solver->synthesis(guard);
    return res;
}

FunctionContext BasicEnumInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);
    auto* solver = new BasicEnumSolver(spec, v);
    auto res = solver->synthesis(guard);
    return res;
}

FunctionContext OBEInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);
    ProgramChecker runnable = [](Program* p) {return true;};

    auto* obe = new OBESolver(spec, v, runnable);
    auto* solver = new CEGISSolver(obe, v);

    auto res = solver->synthesis(guard);
    return res;
}

FunctionContext EuSolverInvoker(Specification* spec, TimeGuard* guard) {
    auto *v = sygus::getVerifier(spec);
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto *solver = new EuSolver(spec, stun_info.first, stun_info.second);
    auto *cegis = new CEGISSolver(solver, v);

    return cegis->synthesis(guard);
}
namespace {
    Example _buildExample(const std::vector<int>& x) {
        Example e;
        for (int w: x) e.push_back(BuildData(Int, w));
        return e;
    }

    IOExample _buildIOExample(const std::vector<int>& x, int y) {
        Example inp = _buildExample(x);
        auto oup = BuildData(Int, y);
        return {inp, oup};
    }
}

FunctionContext LIASolverInvoker(Specification* spec, TimeGuard* guard) {
    IOExampleList example_list;
    example_list.push_back(_buildIOExample({0, 4}, 1));
    example_list.push_back(_buildIOExample({4, 0}, 17));
    auto example_space = example::buildFiniteIOExampleSpace(example_list, "f", spec->env.get());
    spec->example_space = example_space;
    LIASolver* solver = solver::lia::liaSolverBuilder(spec);
    solver = dynamic_cast<LIASolver*>(solver::relaxSolver(solver));

    ExampleList inp_list;
    for (auto& example: example_list) {
        auto now = example.first; now.push_back(example.second);
        inp_list.push_back(now);
    }
    return solver->synthesis(inp_list, guard);
}

FunctionContext PolyGenInvoker(Specification* spec, TimeGuard* guard) {
    auto *v = sygus::getVerifier(spec);
    auto domain_builder = solver::lia::liaSolverBuilder;
    auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* solver = new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, v);

    return solver->synthesis(guard);
}

FunctionContext PolyGenSelectorInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = new Z3SplitSelector(spec, 50);
    auto domain_builder = solver::lia::liaSolverBuilder;
    auto dnf_builder = [](Specification* spec) -> PBESolver* {return new DNFLearner(spec);};
    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* solver = new CEGISPolyGen(spec, stun_info.first, stun_info.second, domain_builder, dnf_builder, v);
    auto res = solver->synthesis(guard);
    return res;
}


void PolyGenTermSolverInvoker(Specification* spec, TimeGuard* guard) {
    ExampleList example_list;
    example_list.push_back(_buildExample({10, 9}));
    example_list.push_back(_buildExample({9, 10}));

    auto stun_info = solver::divideSyGuSSpecForSTUN(spec->info_list[0], spec->env.get());
    auto* term_solver = new PolyGenTermSolver(spec, stun_info.first, solver::lia::liaSolverBuilder);
    auto res = term_solver->synthesisTerms(example_list, guard);
    for (const auto& p: res) {
        LOG(INFO) << p->toString() << std::endl;
    }
    exit(0);
}

auto prepare = [](Specification* spec, const IOExample& io_example) {
    auto* env = spec->env.get(); auto* g = spec->info_list[0]->grammar;
    std::vector<std::string> string_const_list;
    auto add_const = [&](const Data& d) {
        auto* sv = dynamic_cast<StringValue*>(d.get());
        if (sv) string_const_list.push_back(sv->s);
    };
    for (auto* symbol: g->symbol_list) {
        for (auto* rule: symbol->rule_list) {
            auto* sem = dynamic_cast<ConstSemantics*>(rule->semantics.get());
            if (sem) add_const(sem->w);
        }
    }
    for (const auto& inp: io_example.first) add_const(inp);
    add_const(io_example.second);

    DataList const_list; int int_max = 1;
    for (const auto& s: string_const_list) {
        const_list.push_back(BuildData(String, s));
        int_max = std::max(int_max, int(s.length()));
    }

    env->setConst(theory::clia::KWitnessIntMinName, BuildData(Int, -1));
    env->setConst(theory::string::KStringConstList, const_list);
    env->setConst(theory::clia::KWitnessIntMaxName, BuildData(Int, int_max));
};

FunctionContext BasicVSAInvoker(Specification* spec, TimeGuard* guard) {
    if (spec->info_list.size() > 1) {
        LOG(FATAL) << "VSASolver can only synthesize a single program";
    }
    auto info = spec->info_list[0];
    auto* pruner = new SizeLimitPruner(10000, [](VSANode* node) {
        auto* sn = dynamic_cast<SingleVSANode*>(node);
        if (sn) {
            auto* oup = dynamic_cast<DirectWitnessValue*>(sn->oup.get());
            if (oup) {
                auto* w = dynamic_cast<StringValue*>(oup->d.get());
                if (w) return w->s.length() >= 15;
            }
        }
        return false;
    });
    auto* builder = new BFSVSABuilder(info->grammar, pruner, spec->env.get());
    // auto* builder = new DFSVSABuilder(info->grammar, pruner, spec->env);

    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);

    auto* solver = new BasicVSASolver(spec, builder, prepare, ext::vsa::getSizeModel());
    auto* verifier = sygus::getVerifier(spec);
    auto* cegis = new CEGISSolver(solver, verifier);

    auto res = cegis->synthesis(guard);
    return res;
}

FunctionContext MaxFlashInvoker(Specification* spec, TimeGuard* guard) {
    auto* v = sygus::getVerifier(spec);

    sygus::loadSyGuSTheories(spec->env.get(), theory::loadWitnessFunction);

    auto* model = ext::vsa::getSizeModel();
    auto* solver = new MaxFlash(spec, v, model, prepare);
    return solver->synthesis(guard);
}

#include <dirent.h>
#include <cstddef>

std::vector<std::string> getFiles(const std::string& path)
{
    DIR *dir = opendir(path.c_str());
    struct dirent *ptr;
    std::vector<std::string> res;
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;
        else if (ptr->d_type == 8) {
            std::string name = ptr->d_name;
            if (name.find(".sl") == std::string::npos) continue;
            res.push_back(path + ptr->d_name);
        } else if (ptr->d_type == 10)
            continue;
    }
    closedir(dir);
    return res;
}

#include <cmath>

void learnModel() {
    std::string dir = config::KSourcePath + "/tests/string/";
    std::string model_path = config::KSourcePath + "ext/vsa/model.json";
    auto file_list = getFiles(dir);
    ProgramList program_list;
    for (const auto& path: file_list) {
        Specification* spec = parser::getSyGuSSpecFromFile(path);
        Json::Value root = parser::getJsonForSyGuSFile(path);
        for (auto& item: root) {
            if (item[0].asString() == "define-fun") {
                program_list.push_back(json::getProgramFromJson(item, spec->env.get()));
            }
        }
    }
    double default_value = std::log(1000);
    auto model = new NGramModel(1, ext::vsa::KDefaultAbstracter, default_value);
    ext::vsa::learn(model, program_list);
    for (auto& program: program_list) {
        std::cout << program->toString() << " " << dynamic_cast<TopDownModel*>(model)->getWeight(program.get()) << std::endl;
    }
    ext::vsa::saveNGramModel(model, model_path);
}

int main() {
    learnModel(); exit(0);
    // std::string file = config::KSourcePath + "/tests/polygen/secondmin.sl";
    std::string file = config::KSourcePath + "/tests/string/phone-10_short.sl";
    auto* spec = parser::getSyGuSSpecFromFile(file);
    auto* guard = new TimeGuard(500);
    auto res = MaxFlashInvoker(spec, guard);
    //auto res = LIASolverInvoker(spec, guard);
    std::cout << res.toString() << std::endl;
    std::cout << "Time Cost: " << guard->getPeriod() << " seconds";
    return 0;
}
