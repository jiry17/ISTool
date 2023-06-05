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
#include <iostream>

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
        // LOG(INFO) << "CNUM " << c_num;
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

    PType _getCompressType(IncreInfo* info, int compress_id) {
        for (const auto& align_info: info->align_infos) {
            for (auto&[name, ty]: align_info->inp_types) {
                if (ty->getType() == TyType::COMPRESS) {
                    auto *cty = dynamic_cast<TyLabeledCompress *>(ty.get());
                    if (cty && cty->id == compress_id) return incre::typeFromIncre(cty->content);
                }
            }
        }
        LOG(FATAL) << "Compress #" << compress_id << " not found";
    }
}

OutputUnit::OutputUnit(const std::vector<int> &_path, const Ty &_unit_type): path(_path), unit_type(_unit_type) {
}

GrammarEnumerateTool::GrammarEnumerateTool(Grammar *_grammar): grammar(_grammar), size_limit(::grammar::getMaxSize(_grammar)) {
    if (size_limit == -1) size_limit = 1e9;
}

Grammar * IncreAutoLifterSolver::buildAuxGrammar(int compress_id) {
    TypeList inp_list = {_getCompressType(info, compress_id)};
    for (auto& inp_type: global_input_type_list) inp_list.push_back(inp_type);
    return info->component_pool.buildAlignGrammar(inp_list);
}
Grammar * IncreAutoLifterSolver::buildCompressGrammar(const TypeList &type_list, int align_id) {
    int pos = info->align_infos[align_id]->command_id;
    TypeList inp_list = type_list;
    for (auto& inp_type: global_input_type_list) inp_list.push_back(inp_type);
    return info->component_pool.buildCompressGrammar(inp_list, pos);
}
Grammar * IncreAutoLifterSolver::buildCombinatorGrammar(const TypeList &type_list, const PType& oup_type, int align_id) {
    auto feature = std::to_string(align_id) + "@" + type::typeList2String(type_list) + "@" + oup_type->getName();
    if (combine_grammar_map.count(feature)) return combine_grammar_map[feature];
    int pos = info->align_infos[align_id]->command_id;
    return info->component_pool.buildCombinatorGrammar(type_list, oup_type, pos);
}

IncreAutoLifterSolver::IncreAutoLifterSolver(IncreInfo *_info, const PEnv& _env): env(_env), IncreSolver(_info),
    f_res_list(_getCNum(_info)), compress_res_list(info->align_infos.size()),
    aux_grammar_list(_getCNum(_info)) {
    std::cout << "zyw: IncreAutoLifterSolver" << std::endl;
    for (auto& [name, inp_type]: info->example_pool->input_list) {
        global_input_type_list.push_back(incre::typeFromIncre(inp_type));
    }
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

    std::cout << "zyw: print grammar in solver!" << std::endl;
    int num = 0;
    for (auto& grammar_enum: compress_grammar_list) {
        std::cout << "num = " << num++ << std::endl;
        auto& grammar = grammar_enum->grammar;
        grammar->print();
    }
    std::cout << "zyw: aux begin!" << std::endl;
    num = 0;
    for (auto& grammar_enum: aux_grammar_list) {
        std::cout << "num = " << num++ << std::endl;
        auto& grammar = grammar_enum->grammar;
        grammar->print();
    }
    std::cout << "zyw: print grammar end!" << std::endl;
    std::cout << "zyw: print grammar map begin!" << std::endl;
    for (auto& [name, grammar]: combine_grammar_map) {
        std::cout << name << " " << std::endl;
        grammar->print();
    }
    std::cout << "zyw: print grammar map end!" << std::endl;
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

#include "istool/basic/config.h"

IncreSolution IncreAutoLifterSolver::solve() {
    global::recorder.start("syn-align");
    solveAuxiliaryProgram();
    global::recorder.end("syn-align");
    global::recorder.start("syn-comb");
    solveCombinators();
    global::recorder.end("syn-comb");
    return {f_type_list, comb_list};
}

void IncreAutoLifterSolver::printToHaskell() const {
    std::cout << "zyw: print grammar in solver!" << std::endl;
    int num = 0;
    for (auto& grammar_enum: compress_grammar_list) {
        std::cout << "num = " << num++ << std::endl;
        auto& grammar = grammar_enum->grammar;
        grammar->printToHaskell();
    }
    std::cout << "zyw: aux begin!" << std::endl;
    num = 0;
    for (auto& grammar_enum: aux_grammar_list) {
        std::cout << "num = " << num++ << std::endl;
        auto& grammar = grammar_enum->grammar;
        grammar->printToHaskell();
    }
}