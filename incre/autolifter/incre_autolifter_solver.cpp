//
// Created by pro on 2022/9/26.
//

#include <istool/ext/deepcoder/data_type.h>
#include <istool/ext/deepcoder/deepcoder_semantics.h>
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/autolifter/incre_plp_solver.h"
#include "istool/incre/trans/incre_trans.h"
#include "istool/solver/autolifter/basic/streamed_example_space.h"
#include "glog/logging.h"

using namespace incre;
using namespace incre::autolifter;

FInfo::FInfo(const TypedProgram& _program, int _id, bool _is_extended):
    program(_program), id(_id), is_extended(_is_extended) {
}
FExample::FExample(const DataStorage &_compress_storage, const DataList &_const_list, const Data &_oup):
    compress_storage(_compress_storage), const_list(_const_list), oup(_oup) {
}
std::string FExample::toString() const {
    std::string res = "compress: [";
    for (int i = 0; i < compress_storage.size(); ++i) {
        if (i) res += ","; res += data::dataList2String(compress_storage[i]);
    }
    res += "]; const: " + data::dataList2String(const_list) + "; oup: " + oup.toString();
    return res;
}
void FExampleSpace::addExample() {
    auto example = pool->example_pool[tau_id][example_list.size()];
    DataStorage compress_storage;
    auto unwrap_compress = [](Value* value) {
        auto* cv = dynamic_cast<VCompress*>(value);
        assert(cv);
        return cv->content;
    };
    for (auto& [_, name_list]: compress_infos) {
        DataList compress_list;
        for (auto& name: name_list) {
#ifdef DEBUG
            assert(example->inputs.count(name));
#endif
            auto* v = example->inputs[name].get();
            compress_list.push_back(unwrap_compress(v));
        }
        compress_storage.push_back(compress_list);
    }
    DataList const_list;
    for (auto& [name, _]: const_infos) {
#ifdef DEBUG
        assert(example->inputs.count(name));
#endif
        const_list.push_back(example->inputs[name]);
    }
    example_list.emplace_back(compress_storage, const_list, example->oup);
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
        pool(_pool), tau_id(_tau_id), env(_env) {
    std::unordered_map<int, int> used_map;
    auto insert = [&](int id) {
        if (used_map.count(id) == 0) {
            used_map[id] = compress_infos.size();
            compress_infos.emplace_back(id, (std::vector<std::string>){});
        }
        return used_map[id];
    };
    for (auto& [var_name, var_type]: info->inp_types) {
        if (var_type->type == TyType::COMPRESS) {
            auto* tc = dynamic_cast<TyLabeledCompress*>(var_type.get());
            assert(tc); int id = insert(tc->id);
            compress_infos[id].second.push_back(var_name);
        } else {
            const_infos.emplace_back(var_name, incre::typeFromIncre(var_type));
        }
    }
}

namespace {
    int _getCNum(IncreInfo* info) {
        int c_num = 0;
        auto get_id = [](TyData* type) {
            auto* ct = dynamic_cast<TyLabeledCompress*>(type);
            if (ct) return ct->id + 1;
            return 0;
        };
        for (auto& align_info: info->align_infos) {
            for (auto& [_, type]: align_info->inp_types) c_num = std::max(c_num, get_id(type.get()));
            c_num = std::max(c_num, get_id(align_info->oup_type.get()));
        }
        return c_num;
    }

    void _unfoldOutputType(const Ty& type, std::vector<int>& path, std::vector<OutputUnit>& res) {
        if (type->getType() == TyType::TUPLE) {
            auto* tt = dynamic_cast<TyTuple*>(type.get());
            for (int i = 0; i < tt->fields.size(); ++i) {
                path.push_back(i);
                _unfoldOutputType(tt->fields[i], path, res);
                path.pop_back();
            }
        } else {
            res.emplace_back(path, type);
        }
    }

    std::vector<OutputUnit> _unfoldOutputType(const Ty& type) {
        std::vector<OutputUnit> res;
        std::vector<int> path;
        _unfoldOutputType(type, path, res);
        return res;
    }
}

OutputUnit::OutputUnit(const std::vector<int> &_path, const Ty &_unit_type): path(_path), unit_type(_unit_type) {
}

TypeLabeledDirectSemantics::TypeLabeledDirectSemantics(const PType &_type):
    NormalSemantics("@" + _type->getName(), _type, (TypeList){_type}), type(_type) {
}
Data TypeLabeledDirectSemantics::run(DataList &&inp_list, ExecuteInfo *info) {
    return inp_list[0];
}

IncreAutoLifterSolver::IncreAutoLifterSolver(IncreInfo *_info, const PEnv& _env): env(_env), IncreSolver(_info),
    f_res_list(_getCNum(_info)), const_res_list(info->align_infos.size()) {
    for (auto& pass_info: info->align_infos) {
        example_space_list.push_back(new FExampleSpace(info->example_pool, pass_info->getId(), env, pass_info.get()));
        unit_storage.push_back(_unfoldOutputType(pass_info->oup_type));
    }
#ifdef DEBUG
    for (int i = 0; i < info->align_infos.size(); ++i) assert(info->align_infos[i]->getId() == i);
#endif
}
IncreAutoLifterSolver::~IncreAutoLifterSolver() {
    for (auto* example_space: example_space_list) {
        delete example_space;
    }
    for (auto& [name, grammar]: grammar_map) delete grammar;
}
bool FRes::isEqual(Program *x, Program *y) {
    //TODO: add a semantical check
    return x->toString() == y->toString();
}
Data FRes::run(const Data &inp, Env *env) {
    if (component_list.empty()) return Data(std::make_shared<VUnit>());
    if (component_list.size() == 1) {
        return env->run(component_list[0].program.second.get(), {inp});
    }
    DataList elements;
    for (auto& component: component_list) {
        elements.push_back(env->run(component.program.second.get(), {inp}));
    }
    return Data(std::make_shared<VTuple>(elements));
}
int FRes::insert(const TypedProgram& program) {
    for (int i = 0; i < component_list.size(); ++i) {
        auto& info = component_list[i];
        if (type::equal(program.first, info.program.first) && isEqual(program.second.get(), info.program.second.get())) return i;
    }
    int id = component_list.size();
    component_list.emplace_back(program, id, false);
    return id;
}
bool ConstRes::isEqual(Program *x, Program *y) {
    //TODO: add a semantical check
    return x->toString() == y->toString();
}
int ConstRes::insert(const TypedProgram& program) {
    for (int i = 0; i < const_list.size(); ++i) {
        if (type::equal(program.first, const_list[i].first) && isEqual(program.second.get(), const_list[i].second.get())) return i;
    }
    int id = const_list.size();
    const_list.push_back(program);
    return id;
}

namespace {
    Data _run(Program* program, const DataList& inp, Env* env) {
        return env->run(program, inp);
    }
}

DataList FExampleSpace::runAux(int example_id, int id, Program *program) {
    auto& example = example_list[example_id];
    DataList res;
    for (auto& inp: example.compress_storage[id]) {
        res.push_back(_run(program, {inp}, env.get()));
    }
    return res;
}
Data FExampleSpace::runConst(int example_id, Program *program) {
    auto& example = example_list[example_id];
    return _run(program, example.const_list, env.get());
}

namespace {
    Data _extract(const Data& d, const std::vector<int>& path) {
        Data res(d);
        for (auto pos: path) {
            auto* v = dynamic_cast<VTuple*>(res.get());
            assert(v && v->elements.size() > pos);
            res = v->elements[pos];
        }
        auto* cv = dynamic_cast<VCompress*>(res.get());
        if (cv) return cv->content;
        return res;
    }
}
Data FExampleSpace::runOup(int example_id, Program *program, const std::vector<int>& path) {
    auto oup = _extract(example_list[example_id].oup, path);
    if (program) {
        return _run(program, {oup}, env.get());
    }
    return oup;
}

autolifter::PLPRes IncreAutoLifterSolver::solvePLPTask(AlignTypeInfoData *info, const TypedProgram &target, const std::vector<int>& path, int target_id) {
    auto* space = example_space_list[info->getId()];
    std::vector<Grammar*> f_grammar_list;
    for (auto& [id, _]: space->compress_infos) {
        auto* grammar = buildAuxGrammar(id);
        // LOG(INFO) << "grammar for " << id;
        // grammar->print();
        f_grammar_list.push_back(grammar);
    }
    TypeList const_types;
    for (auto& [_, type]: space->const_infos) {
        const_types.push_back(type);
    }
    auto* const_grammar = buildConstGrammar(const_types, info->getId());
    // const_grammar->print();

    auto* task = new PLPTask(space, f_grammar_list, const_grammar, target, path, target_id);
    auto* solver = new IncrePLPSolver(env.get(), task);
    auto res = solver->synthesis(nullptr);

    delete solver; delete task; delete const_grammar;
    for (auto* g: f_grammar_list) delete g;
    return res;
}

void IncreAutoLifterSolver::solveAuxiliaryProgram() {
    auto record_res = [&](int pass_id, const PLPRes& res) {
        auto* example_space = example_space_list[pass_id];
        for (auto& [pos_id, info]: res.first) {
            int f_id = example_space->compress_infos[pos_id].first;
            f_res_list[f_id].insert(info);
        }
        for (auto& const_program: res.second) {
            const_res_list[pass_id].insert(const_program);
        }
    };

    for (auto& align_info: info->align_infos) {
        unit_storage.push_back(_unfoldOutputType(align_info->oup_type));
    }

    for (auto& align_info: info->align_infos) {
        for (auto& unit: unit_storage[align_info->getId()]) {
            auto *oup_ty = unit.unit_type.get();
            if (!dynamic_cast<TyCompress *>(oup_ty)) {
                PLPRes res = solvePLPTask(align_info.get(), {incre::typeFromIncre(unit.unit_type), nullptr}, unit.path, -1);
                record_res(align_info->getId(), res);
            }
        }
    }

    bool is_changed = true;
    while (is_changed) {
        is_changed = false;
        for (int compress_id = 0; compress_id < f_res_list.size(); ++compress_id) {
            for (int i = 0; i < f_res_list[compress_id].component_list.size(); ++i) {
                auto component = f_res_list[compress_id].component_list[i];
                if (component.is_extended) continue;
                is_changed = true;
                f_res_list[compress_id].component_list[i].is_extended = true;
                for (auto& align_info: info->align_infos) {
                    for (auto& unit: unit_storage[align_info->getId()]) {
                        auto *cty = dynamic_cast<TyLabeledCompress *>(unit.unit_type.get());
                        if (cty && cty->id == compress_id) {
                            PLPRes res = solvePLPTask(align_info.get(), component.program, unit.path, compress_id);
                            record_res(align_info->getId(), res);
                        }
                    }
                }
            }
        }
    }

    // build type list
    for (auto& f_res: f_res_list) {
        if (f_res.component_list.empty()) {
            f_type_list.push_back(std::make_shared<TyUnit>()); continue;
        }
        TyList fields;
        for (auto& component: f_res.component_list) fields.push_back(incre::typeToIncre(component.program.first.get()));
        if (fields.size() == 1) {
            f_type_list.push_back(fields[0]);
        } else {
            f_type_list.push_back(std::make_shared<TyTuple>(fields));
        }
    }
}

IncreSolution IncreAutoLifterSolver::solve() {
    solveAuxiliaryProgram(); solveCombinators();
    return {f_type_list, comb_list};
}