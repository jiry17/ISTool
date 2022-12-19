//
// Created by pro on 2022/11/24.
//

#ifndef ISTOOL_INCRE_LOOKUP_H
#define ISTOOL_INCRE_LOOKUP_H

#include "incre_program.h"

namespace incre::match {
    typedef std::unordered_map<std::string, bool> MatchContext;

    struct MatchTask {
        std::function<bool(TermData*, const MatchContext&)> term_matcher;
        std::function<bool(TyData*, const MatchContext&)> type_matcher;
        MatchTask();
    };

    bool match(TyData* type, const MatchTask& task);
    bool match(TermData* term, const MatchTask& task);
    MatchContext match(ProgramData* program, const MatchTask& task);
}

namespace incre {
    struct InputComponentInfo {
        std::string name;
        bool is_compress_related, is_recursive;
        int command_id;
        InputComponentInfo(const std::string& _name, bool _is_compress_related, bool _is_recursive, int _command_id);
        InputComponentInfo(const std::string& _name);
        InputComponentInfo() = default;
        ~InputComponentInfo() = default;
    };
    std::unordered_map<std::string, InputComponentInfo> getComponentInfo(const IncreProgram& program);
}

#endif //ISTOOL_INCRE_LOOKUP_H
