//
// Created by pro on 2021/12/8.
//

#include "parser.h"
#include "basic/basic/config.h"
#include "basic/ext/z3/z3_util.h"
#include "basic/ext/z3/z3_verifier.h"
#include "../json_util.h"
#include "glog/logging.h"

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
        LOG(FATAL) << "Unknown Theory " << name;
    }

    PSynthInfo parseSynthInfo(const Json::Value& entry) {
        if (entry.size() != 6) {
            LOG(FATAL) << "Unsupported format " << entry;
        }
        TEST_PARSER(entry[1].isString() && entry[2].isArray() && entry[4].isArray() && entry[5].isArray())
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

        // grammar
        for (auto& nt_node: entry[5]) {
            TEST_PARSER(nt_node.isArray() && nt_node.size() >= 3 && nt_node[2].isArray() && nt_node[0].isString());
            auto* symbol = nt_map[nt_node[0].asString()];
            for (auto& rule_node: nt_node[2]) {
                // try inp
                if (rule_node.isString()) {
                    int inp_id = get_inp_id(rule_node.asString());
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
                        auto s = semantics::string2Semantics(rule_node[0].asString());
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

    PExample parseIOExample(const Json::Value& value, const std::string& name) {
        TEST_PARSER(value.isArray() && value.size() == 2)
        auto example_node = value[0];
        TEST_PARSER(example_node.isArray() && example_node.size() == 3)
        auto op_node = example_node[0], fun_call_node = example_node[1], oup_node = example_node[2];
        TEST_PARSER(op_node.isString() && op_node.asString() == "=")
        TEST_PARSER(fun_call_node.isArray() && fun_call_node[0].isString() && fun_call_node[0].asString() == name)
        DataList inp_list;
        for (int i = 1; i < fun_call_node.size(); ++i) {
            inp_list.push_back(json::getDataFromJson(value));
        }
        return std::make_shared<IOExample>(std::move(inp_list), std::move(json::getDataFromJson(oup_node)));
    }

    z3::expr buildZ3Cons(const Json::Value& entry, const std::map<std::string, z3::expr>& var_map,
            const std::map<std::string, PSynthInfo>& info_map, std::map<std::string, Z3CallInfo>& call_info_map,
            z3::context& ctx) {
        if (entry.isString()) {
            std::string name = entry.asString();
            TEST_PARSER(var_map.find(name) != var_map.end());
            return var_map.find(name)->second;
        }
        TEST_PARSER(entry.isArray())
        try {
            auto data = json::getDataFromJson(entry);
            auto* z3v = dynamic_cast<Z3Value*>(data.get());
            TEST_PARSER(z3v);
            return z3v->getZ3Expr(ctx);
        } catch (ParseError& e) {
        }
        TEST_PARSER(entry[0].isString())
        std::string name = entry[0].asString();
        if (info_map.find(name) != info_map.end()) {
            auto syn_info = info_map.find(name)->second;
            std::string feature = entry.toStyledString();
            if (call_info_map.find(feature) != call_info_map.end()) {
                return call_info_map.find(feature)->second.oup_symbol;
            }
            std::string oup_name = "oup" + std::to_string(call_info_map.size());
            auto oup_var = ext::buildVar(syn_info->oup_type, oup_name, ctx);
            z3::expr_vector inp_list(ctx);
            for (int i = 1; i < entry.size(); ++i) {
                inp_list.push_back(buildZ3Cons(entry[i], var_map, info_map, call_info_map, ctx));
            }
            call_info_map.insert({feature, Z3CallInfo(feature, inp_list, oup_var)});
            return oup_var;
        }
        auto z3s = ext::getZ3Semantics(name);
        z3::expr_vector inp_list(ctx);
        for (int i = 1; i < entry.size(); ++i) {
            inp_list.push_back(buildZ3Cons(entry[i], var_map, info_map, call_info_map, ctx));
        }
        return z3s->encodeZ3Expr(inp_list);
    }

}

Json::Value parser::getJsonForSyGuSFile(const std::string &file_name) {
    std::string oup_file = "/tmp/" + std::to_string(rand()) + ".json";
    std::string python_path = config::KSourcePath + "basic/parser/sygus/python";
    std::string command = "cd " + python_path + "; python3 main.py " + file_name + " " + oup_file;
    system(command.c_str());
    Json::Value root = json::loadJsonFromFile(oup_file);
    command = "rm " + oup_file;
    system(command.c_str());
    return root;
}

TheorySpecification * parser::getSyGuSSpecFromFile(const std::string &file_name) {
    auto root = getJsonForSyGuSFile(file_name);
    return getSyGuSSpecFromJson(root);
}

TheorySpecification * parser::getSyGuSSpecFromJson(const Json::Value& root) {
    auto theory_list = getEntriesViaName(root, "set-logic");
    assert(theory_list.size() == 1);
    auto theory = getTheory(theory_list[0][1].asString());
    auto assigner = semantics::loadTheory(theory);
    auto* type_system = new BasicTypeSystem(assigner);

    auto fun_list = getEntriesViaName(root, "synth-fun");
    std::vector<PSynthInfo> info_list;
    for (auto& fun_info: fun_list) {
        info_list.push_back(parseSynthInfo(fun_info));
    }

    auto cons_list = getEntriesViaName(root, "constraint");

    // try example based
    try {
        TEST_PARSER(fun_list.size() == 1);
        std::string name = info_list[0]->name;
        std::vector<PExample> example_list;
        for (auto& entry: cons_list) {
            example_list.push_back(parseIOExample(entry, name));
        }
        auto example_space = std::make_shared<FiniteExampleSpace>(example_list);
        auto* verifier = new FiniteExampleVerifier(example_space);
        return new TheorySpecification(info_list, type_system, verifier, theory);
    } catch (ParseError& e) {
    }

    // try z3 based
    ext::loadZ3Theory(theory);
    try {
        auto declare_var_list = getEntriesViaName(root, "declare-var");
        std::map<std::string, z3::expr> var_map;
        for (auto& entry: declare_var_list) {
            std::string name = entry[1].asString();
            PType type = json::getTypeFromJson(entry[2]);
            var_map.insert({name, ext::buildVar(type, name, ext::z3_ctx)});
        }
        std::map<std::string, PSynthInfo> info_map;
        for (const auto& info: info_list) {
            info_map.insert({info->name, info});
        }
        std::map<std::string, Z3CallInfo> call_info_map;
        z3::expr_vector z3_cons_list(ext::z3_ctx);
        for (const auto& entry: cons_list) {
            z3_cons_list.push_back(buildZ3Cons(entry[1], var_map, info_map, call_info_map, ext::z3_ctx));
        }
        std::vector<Z3CallInfo> call_info_list;
        for (const auto& info: call_info_map) call_info_list.push_back(info.second);
        if (call_info_map.size() > 1 || info_map.size() > 1) {
            LOG(FATAL) << "Do not support specifications with multiple IOs";
        }
        auto* verifier = new Z3IOVerifier(z3_cons_list, call_info_list[0]);
        return new TheorySpecification(info_list, type_system, verifier, theory);
    } catch (ParseError& e) {
    }
    LOG(FATAL) << "Unknown specification";
}