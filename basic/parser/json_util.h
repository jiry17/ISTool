//
// Created by pro on 2021/12/5.
//

#ifndef ISTOOL_JSON_UTIL_H
#define ISTOOL_JSON_UTIL_H

#include "basic/basic/data.h"
#include "json/json.h"
#include <exception>

struct ParseError: public std::exception {
};

#define TEST_PARSER(b) if (!(b)) throw ParseError();

namespace json {
    Json::Value loadJsonFromFile(const std::string& name);
    PType getTypeFromJson(const Json::Value& value);
    Data getDataFromJson(const Json::Value& value);
}


#endif //ISTOOL_JSON_UTIL_H
