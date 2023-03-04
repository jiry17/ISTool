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
        LOG(INFO) << "CNUM " << c_num;
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

GrammarEnumerateTool::GrammarEnumerateTool(Grammar *_grammar): grammar(_grammar), size_limit(grammar::getMaxSize(_grammar)) {
    if (size_limit == -1) size_limit = 1e9;
}

IncreAutoLifterSolver::IncreAutoLifterSolver(IncreInfo *_info, const PEnv& _env): env(_env), IncreSolver(_info),
    f_res_list(_getCNum(_info)), compress_res_list(info->align_infos.size()),
    aux_grammar_list(_getCNum(_info)) {
    for (auto& align_info: info->align_infos) {
        assert(align_info->getId() == example_space_list.size());
        auto* example_space = new FExampleSpace(info->example_pool, align_info->getId(), env, align_info.get());
        example_space_list.push_back(example_space);
        unit_storage.push_back(_unfoldOutputType(align_info->oup_type));

        TypeList inp_list;
        for (auto& [_, t]: example_space->value_list) inp_list.push_back(t);
        compress_grammar_list.push_back(new GrammarEnumerateTool(buildCompressGrammar(inp_list, align_info->getId())));
    }
    for (int i = 0; i < aux_grammar_list.size(); ++i) {
        aux_grammar_list[i] = new GrammarEnumerateTool(buildAuxGrammar(i));
    }
#ifdef DEBUG
    for (int i = 0; i < info->align_infos.size(); ++i) assert(info->align_infos[i]->getId() == i);
#endif
}
IncreAutoLifterSolver::~IncreAutoLifterSolver() {
    for (auto* example_space: example_space_list) {
        delete example_space;
    }
    for (auto* g: aux_grammar_list) delete g;
    for (auto* g: compress_grammar_list) delete g;
    for (auto& [_, grammar]: combine_grammar_map) delete grammar;
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

bool CompressRes::isEqual(Program* x, Program* y) {
    // TODO: add a semantical check
    return x->toString() == y->toString();
}
int CompressRes::insert(const TypedProgram& program) {
    for (int i = 0; i < compress_list.size(); ++i) {
        if (type::equal(program.first, compress_list[i].first) && isEqual(program.second.get(), compress_list[i].second.get())) return i;
    }
    int id = int(compress_list.size()); compress_list.push_back(program);
    return id;
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
    auto oup = _extract(example_list[example_id].second, path);
    if (program) {
        return env->run(program, {oup});
    }
    return oup;
}

autolifter::PLPRes IncreAutoLifterSolver::solvePLPTask(AlignTypeInfoData *info, const TypedProgram &target, const std::vector<int>& path) {
    auto* space = example_space_list[info->getId()];

    std::vector<TypedProgramList> known_lifts(aux_grammar_list.size());
    for (int i = 0; i < aux_grammar_list.size(); ++i) {
        for (auto& component: f_res_list[i].component_list) {
            known_lifts[i].push_back(component.program);
        }
    }

    auto* task = new PLPTask(space, aux_grammar_list, known_lifts, compress_grammar_list[info->getId()], target, path);
    auto* solver = new IncrePLPSolver(env.get(), task);
    auto res = solver->synthesis(nullptr);

    delete solver; delete task;
    return res;
}

void IncreAutoLifterSolver::solveAuxiliaryProgram() {
    auto record_res = [&](int align_id, const PLPRes& res) {
        for (auto& [compress_program, aux_program]: res) {
            auto* ltc = dynamic_cast<TLabeledCompress*>(compress_program.first.get());
            if (ltc) {
                f_res_list[ltc->id].insert(aux_program);
            }
            // LOG(INFO) << align_id <<"  " << compress_res_list.size() << " " << compress_program.first << " " << compress_program.second;
            // LOG(INFO) << compress_program.first->getName() + "@" + compress_program.second->toString();
            compress_res_list[align_id].insert(compress_program);
        }
    };

    for (auto& align_info: info->align_infos) {
        unit_storage.push_back(_unfoldOutputType(align_info->oup_type));
    }

    for (auto& align_info: info->align_infos) {
        for (auto& unit: unit_storage[align_info->getId()]) {
            auto *oup_ty = unit.unit_type.get();
            if (!dynamic_cast<TyCompress *>(oup_ty)) {
                PLPRes res = solvePLPTask(align_info.get(), {incre::typeFromIncre(unit.unit_type), nullptr}, unit.path);
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
                            PLPRes res = solvePLPTask(align_info.get(), component.program, unit.path);
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