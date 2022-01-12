//
// Created by pro on 2021/12/8.
//

#include "istool/sygus/parser/parser.h"
#include "istool/sygus/parser/json_util.h"
#include "istool/sygus/theory/basic/theory_semantics.h"
#include "istool/sygus/theory/z3/theory_z3_semantics.h"
#include "istool/basic/config.h"
#include "istool/ext/z3/z3_example_space.h"
#include "glog/logging.h"
#include <unordered_set>

namespace {
    std::vector<Json::Value> getEntriesViaName(const Json::Value& root, const std::string& name) {
        assert(root.isArray());
        std::vector<Json::Value> res;
        for (auto& entry: root) {
            assert(entry.isArray() && entry[0].isString());
            if (entry[0].asString() == name) {
                res.push_back(entry);
            }
        }
        return res;
    }

    TheoryToken getTheory(const std::string& name) {
        if (name == "LIA") return TheoryToken::CLIA;
        if (name == "SLIA") return TheoryToken::STRING;
        LOG(FATAL) << "Unknown Theory " << name;
    }

    PSynthInfo parseSynthInfo(const Json::Value& entry, Env* env) {
        if (entry.size() != 5 && entry.size() != 6) {
            LOG(FATAL) << "Unsupported format " << entry;
        }
        TEST_PARSER(entry[1].isString() && entry[2].isArray() && entry[4].isArray())
        std::string name = entry[1].asString();

        // inp types
        TypeList inp_type_list;
        std::vector<std::string> inp_name_list;
        for (auto& inp_item: entry[2]) {
            TEST_PARSER(inp_item.isArray() && inp_item.size() == 2 && inp_item[0].isString());
            inp_name_list.push_back(inp_item[0].asString());
            inp_type_list.push_back(json::getTypeFromJson(inp_item[1]));
        }
        auto get_inp_id = [=](const std::string& name) {
            for (int i = 0; i < inp_name_list.size(); ++i) {
                if (inp_name_list[i] == name) return i;
            }
            return -1;
        };

        // oup_type
        PType oup_type = json::getTypeFromJson(entry[3]);

        // nt_map
        std::map<std::string, NonTerminal*> nt_map;
        NTList symbol_list;
        for (auto& nt_node: entry[4]) {
            TEST_PARSER(nt_node.isArray() && nt_node[0].isString());
            std::string symbol_name = nt_node[0].asString();
            auto* symbol = new NonTerminal(symbol_name, json::getTypeFromJson(nt_node[1]));
            nt_map[symbol_name] = symbol;
            symbol_list.push_back(symbol);
        }

        int grammar_pos = entry.size() - 1;
        // The following is a special treatment for rule Start -> (Symbol).
        // At this time, Symbol can be regarded as the start symbol and Start can be removed.
        // TODO: make this part more general
        std::string start_name = symbol_list[0]->name;
        bool is_used_start = false, is_start_dummy = false;
        std::string replace_name;
        for (auto& nt_node: entry[grammar_pos]) {
            auto nt_name = nt_node[0].asString();
            for (auto& rule_node: nt_node[2]) {
                if (rule_node.isString()) {
                    if (rule_node.asString() == start_name) {
                        is_used_start = true;
                    }
                } else if (rule_node.isArray()) {
                    for (auto& sub_node: rule_node)
                        if (sub_node.isString() && sub_node.asString() == start_name) {
                            is_used_start = true;
                        }
                }
            }
            if (nt_name == start_name) {
                if (nt_node[2].size() != 1) continue;
                auto rule_node = nt_node[2][0];
                if (!rule_node.isString()) continue;
                replace_name = rule_node.asString();
                TEST_PARSER(nt_map.find(replace_name) != nt_map.end())
                is_start_dummy = true;
            }
        }

        if (is_start_dummy) {
            TEST_PARSER(!is_used_start);
            int pos = -1;
            for (int i = 1; i < symbol_list.size(); ++i) {
                if (symbol_list[i]->name == replace_name) {
                    pos = i; break;
                }
            }
            TEST_PARSER(pos != -1);
            std::swap(symbol_list[0], symbol_list[pos]);
            for (int i = pos; i + 1 < symbol_list.size(); ++i) {
                symbol_list[i] = symbol_list[i + 1];
            }
            symbol_list.resize(symbol_list.size() - 1);
        }

        // grammar
        for (auto& nt_node: entry[grammar_pos]) {
            TEST_PARSER(nt_node.isArray() && nt_node.size() >= 3 && nt_node[2].isArray() && nt_node[0].isString());
            auto symbol_name = nt_node[0].asString();
            if (is_start_dummy && symbol_name == start_name) {
                continue;
            }
            auto* symbol = nt_map[symbol_name];
            for (auto& rule_node: nt_node[2]) {
                // try inp
                if (rule_node.isString()) {
                    int inp_id = get_inp_id(rule_node.asString());
                    if (inp_id == -1) LOG(FATAL) << "Cannot find " << rule_node << std::endl;
                    TEST_PARSER(inp_id >= 0)
                    PType inp_type = inp_type_list[inp_id];
                    auto s = semantics::buildParamSemantics(inp_id, std::move(inp_type));
                    symbol->rule_list.push_back(new Rule(std::move(s), {}));
                } else {
                    TEST_PARSER(rule_node.isArray())
                    try {
                        Data d = json::getDataFromJson(rule_node);
                        auto s = semantics::buildConstSemantics(d);
                        symbol->rule_list.push_back(new Rule(std::move(s), {}));
                    } catch (ParseError& e) {
                        TEST_PARSER(rule_node[0].isString())
                        auto s = env->getSemantics(rule_node[0].asString());
                        NTList param_list;
                        for (int i = 1; i < rule_node.size(); ++i) {
                            TEST_PARSER(rule_node[i].isString())
                            std::string param_name = rule_node[i].asString();
                            TEST_PARSER(nt_map.find(param_name) != nt_map.end())
                            param_list.push_back(nt_map[param_name]);
                        }
                        symbol->rule_list.push_back(new Rule(std::move(s), std::move(param_list)));
                    }
                }
            }
        }
        auto* grammar = new Grammar(symbol_list[0], symbol_list);
        return std::make_shared<SynthInfo>(name, inp_type_list, oup_type, grammar);
    }

    PProgram buildConsProgram(const Json::Value& entry, const std::map<std::string, std::pair<int, PType>>& var_info_map,
            const std::map<std::string, PSynthInfo>& syn_info_map, std::map<std::string, PProgram>& cache, Env* env) {
        auto feature = entry.toStyledString();
        if (cache.find(feature) != cache.end()) return cache[feature];
        if (entry.isString()) {
            std::string name = entry.asString();
            TEST_PARSER(var_info_map.find(name) != var_info_map.end())
            auto var_info = var_info_map.find(name)->second;
            return cache[feature] = program::buildParam(var_info.first, var_info.second);
        }
        TEST_PARSER(entry.isArray())
        try {
            auto data = json::getDataFromJson(entry);
            return cache[feature] = program::buildConst(data);
        } catch (ParseError& e) {
        }
        TEST_PARSER(entry[0].isString())
        ProgramList sub_list;
        for (int i = 1; i < entry.size(); ++i) {
            sub_list.push_back(buildConsProgram(entry[i], var_info_map, syn_info_map, cache, env));
        }
        std::string name = entry[0].asString();
        PSemantics sem;
        if (syn_info_map.find(name) != syn_info_map.end()) {
            auto syn_info = syn_info_map.find(name)->second;
            PType oup_type = syn_info->oup_type;
            TypeList inp_type_list = syn_info->inp_type_list;
            sem = std::make_shared<InvokeSemantics>(syn_info->name, std::move(oup_type), std::move(inp_type_list));
        } else {
            sem = env->getSemantics(name);
        }
        return cache[feature] = std::make_shared<Program>(std::move(sem), std::move(sub_list));
    }

    IOExample parseIOExample(const Json::Value& node, const PSynthInfo& info) {
        auto main = node[1];
        auto op = main[0].asString(); auto l = main[1]; auto r = main[2];
        if (!l.isArray() || !l[0].isString() || l[0].asString() != info->name) std::swap(l, r);
        TEST_PARSER(op == "=" && l.isArray() && l[0].isString() && l[0].asString() == info->name)
        auto oup = json::getDataFromJson(r);
        DataList inp_list;
        for (int i = 1; i < l.size(); ++i) {
            inp_list.push_back(json::getDataFromJson(l[i]));
        }
        return {inp_list, oup};
    }
}

Json::Value parser::getJsonForSyGuSFile(const std::string &file_name) {
    std::string oup_file = "/tmp/" + std::to_string(rand()) + ".json";
    std::string python_path = config::KSourcePath + "/sygus/parser/python";
    std::string command = "cd " + python_path + "; python3 main.py " + file_name + " " + oup_file;
    system(command.c_str());
    Json::Value root = json::loadJsonFromFile(oup_file);
    command = "rm " + oup_file;
    system(command.c_str());
    return root;
}

Specification * parser::getSyGuSSpecFromFile(const std::string &file_name) {
    auto root = getJsonForSyGuSFile(file_name);
    return getSyGuSSpecFromJson(root);
}

namespace {
    void _collectAllNames(const Json::Value& cons, std::unordered_set<std::string>& name_set) {
        if (cons.isString()) {
            name_set.insert(cons.asString()); return;
        }
        try {
            json::getDataFromJson(cons);
            return;
        } catch (ParseError& e) {}
        if (cons.isArray()) {
            for (const auto& sub_node: cons) {
                _collectAllNames(sub_node, name_set);
            }
        }
    }

    std::vector<Json::Value> _removeUselessVars(const std::vector<Json::Value>& var_list,
            const std::vector<Json::Value>& cons_list) {
        std::unordered_set<std::string> name_map;
        for (const auto& cons_node: cons_list) {
            _collectAllNames(cons_node[1], name_map);
        }
        std::vector<Json::Value> result;
        for (const auto& var_node: var_list) {
            std::string var_name = var_node[1].asString();
            if (name_map.find(var_name) != name_map.end()) {
                result.push_back(var_node);
            }
        }
        return result;
    }

    Json::Value _unfoldFunction(const Json::Value& value, const std::unordered_map<std::string, Json::Value>& func_map) {
        if (value.isString()) {
            auto name = value.asString();
            if (func_map.find(name) != func_map.end()) {
                return func_map.find(name)->second;
            }
            return value;
        }
        try {
            auto data = json::getDataFromJson(value);
            return value;
        } catch (ParseError& e) {}
        Json::Value result;
        for (const auto& sub_node: value) {
            result.append(_unfoldFunction(sub_node, func_map));
        }
        return result;
    }

    Json::Value _unfoldFunction(const std::vector<Json::Value>& input_list, const Json::Value& func_spec) {
        auto param_list = func_spec[2];
        std::unordered_map<std::string, Json::Value> param_map;
        for (int i = 0; i < param_list.size(); ++i) {
            auto name = param_list[i][0].asString();
            param_map[name] = input_list[i];
        }
        return _unfoldFunction(func_spec[4], param_map);
    }

    Json::Value _replaceAuxFunctions(const Json::Value& cons, const std::unordered_map<std::string, Json::Value>& func_map) {
        if (cons.isString()) return cons;
        try {
            auto data = json::getDataFromJson(cons);
            return cons;
        } catch (ParseError& e) {}
        assert(cons.isArray() && cons.size() > 1);
        std::string name = cons[0].asString();
        if (func_map.find(name) != func_map.end()) {
            std::vector<Json::Value> param_list;
            for (int i = 1; i < cons.size(); ++i) {
                param_list.push_back(_replaceAuxFunctions(cons[i], func_map));
            }
            return _replaceAuxFunctions(_unfoldFunction(param_list, func_map.find(name)->second), func_map);
        }
        Json::Value res;
        for (const auto& sub_node: cons) {
            res.append(_replaceAuxFunctions(sub_node, func_map));
        }
        return res;
    }
}

Specification * parser::getSyGuSSpecFromJson(const Json::Value& root) {
    auto env = std::make_shared<Env>();

    auto theory_list = getEntriesViaName(root, "set-logic");
    assert(theory_list.size() == 1);
    auto theory = getTheory(theory_list[0][1].asString());
    sygus::initSyGuSExtension(env.get(), theory);
    sygus::loadSyGuSTheories(env.get(), theory::loadBasicSemantics);

    auto fun_list = getEntriesViaName(root, "synth-fun");
    std::vector<PSynthInfo> info_list;
    for (auto& fun_info: fun_list) {
        info_list.push_back(parseSynthInfo(fun_info, env.get()));
    }
    std::map<std::string, PSynthInfo> syn_info_map;
    for (auto& syn_info: info_list) {
        syn_info_map[syn_info->name] = syn_info;
    }

    auto cons_list = getEntriesViaName(root, "constraint");
    auto declare_var_list = getEntriesViaName(root, "declare-var");
    declare_var_list = _removeUselessVars(declare_var_list, cons_list);

    if (declare_var_list.empty()) {
        // try build PBE spec
        try {
            IOExampleList example_list;
            TEST_PARSER(info_list.size() == 1)
            auto syn_info = info_list[0];
            std::string name = syn_info->name;
            for (auto& cons: cons_list) {
                // LOG(INFO) << "Parse example " << cons;
                example_list.push_back(parseIOExample(cons, syn_info));
            }
            auto example_space = example::buildFiniteIOExampleSpace(example_list, name, env.get());
            return new Specification(info_list, env, example_space);
        } catch (ParseError& e) {
        }
    }

    sygus::loadSyGuSTheories(env.get(), theory::loadZ3Semantics);

    auto declare_fun_list = getEntriesViaName(root, "define-fun");
    std::unordered_map<std::string, Json::Value> func_map;
    for (auto& entry: declare_fun_list) {
        auto name = entry[1].asString();
        func_map[name] = entry;
    }

    TypeList var_type_list;
    std::map<std::string, std::pair<int, PType>> var_info_map;
    for (auto& var_info: declare_var_list) {
        int id = var_type_list.size();
        auto name = var_info[1].asString();
        auto type = json::getTypeFromJson(var_info[2]);
        var_type_list.push_back(type);
        var_info_map[name] = {id, type};
    }
    ProgramList cons_program_list;
    std::map<std::string, PProgram> cache;
    for (auto& entry: cons_list) {
        auto cons = _replaceAuxFunctions(entry[1], func_map);
        cons_program_list.push_back(buildConsProgram(cons, var_info_map, syn_info_map, cache, env.get()));
    }
    if (cons_program_list.empty()) {
        LOG(FATAL) << "Constraint should not be empty";
    }
    auto merged_program = cons_program_list[0];
    for (int i = 1; i < cons_program_list.size(); ++i) {
        ProgramList sub_list = {merged_program, cons_program_list[i]};
        merged_program = std::make_shared<Program>(env->getSemantics("&&"), sub_list);
    }

    std::unordered_map<std::string, Signature> sig_map;
    for (auto& info: info_list) {
        sig_map[info->name] = {info->inp_type_list, info->oup_type};
    }

    auto example_space = example::buildZ3ExampleSpace(merged_program, env.get(), var_type_list, sig_map);
    return new Specification(info_list, env, example_space);
}