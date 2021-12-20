//
// Created by pro on 2021/12/5.
//

#include "json_util.h"
#include "istool/sygus/theory/sygus_theory.h"
#include <fstream>
#include <sstream>
#include "glog/logging.h"

Json::Value json::loadJsonFromFile(const std::string &name) {
    Json::Reader reader;
    Json::Value root;
    std::ifstream inp(name, std::ios::out);
    std::stringstream buf;
    buf << inp.rdbuf();
    inp.close();
    assert(reader.parse(buf.str(), root));
    return root;
}

PType json::getTypeFromJson(const Json::Value &value) {
    if (value.isString()) {
        std::string name = value.asString();
        if (name == "Int") return std::make_shared<TInt>();
        if (name == "Bool") return std::make_shared<TBool>();
        if (name == "String") {
            LOG(FATAL) << "Unfinished type: String";
        }
        TEST_PARSER(false)
    }
    TEST_PARSER(value.isArray());
    if (value.size() == 3 && value[0].isString() && value[1].isString() &&
        value[0].asString() == "_" && value[1].asString() == "BitVec") {
        LOG(FATAL) << "Unfinished type: BitVec";
    }
    TEST_PARSER(false)
}

Data json::getDataFromJson(const Json::Value &value) {
    TEST_PARSER(value.isArray()) TEST_PARSER(value[0].isString())
    std::string type = value[0].asString();
    if (type == "Int") {
        TEST_PARSER(value.size() == 2 && value[1].isInt())
        return Data(std::make_shared<IntValue>(value[1].asInt()));
    } else if (type == "Bool") {
        TEST_PARSER(value.size() == 2 && value[1].isString());
        return Data(std::make_shared<BoolValue>(value[1].asString() == "true"));
    } else if (type == "String") {
        LOG(FATAL) << "Unfinished type: String";
    } else if (type == "BitVec") {
        LOG(FATAL) << "Unfinished type: BitVec";
    }
    TEST_PARSER(false);
}