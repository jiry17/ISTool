//
// Created by pro on 2021/12/5.
//

#include "istool/sygus/parser/json_util.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "istool/sygus/theory/basic/string/str.h"
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
        if (name == "String") return std::make_shared<TString>();
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
        return BuildData(Int, value[1].asInt());
    } else if (type == "Bool") {
        TEST_PARSER(value.size() == 2 && value[1].isString())
        return BuildData(Bool, value[1].asString() == "true");
    } else if (type == "String") {
        TEST_PARSER(value.size() == 2 && value[1].isString());
        return BuildData(String, value[1].asString());
    } else if (type == "BitVec") {
        LOG(FATAL) << "Unfinished type: BitVec";
    }
    TEST_PARSER(false);
}

void json::saveJsonToFile(const Json::Value& value, const std::string& file_path) {
    auto* file = fopen(file_path.c_str(), "w");
    fprintf(file, "%s\n", value.toStyledString().c_str());
    fclose(file);
}