//
// Created by pro on 2021/12/8.
//

#ifndef ISTOOL_PARSER_H
#define ISTOOL_PARSER_H

#include "../../theory/theory.h"
#include "json/json.h"

namespace parser {
    Json::Value getJsonForSyGuSFile(const std::string& file_name);
    TheorySpecification* getSyGuSSpecFromJson(const Json::Value& value);
    TheorySpecification* getSyGuSSpecFromFile(const std::string& file_name);
}


#endif //ISTOOL_PARSER_H
