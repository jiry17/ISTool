//
// Created by pro on 2021/12/5.
//

#include "istool/basic/config.h"

#ifdef LINUX
const std::string config::KSourcePath = "/home/jiry/2023S/ISTool/";
#else
const std::string config::KSourcePath = "/home/jiry/zyw/ISTool_230605/";
#endif
const std::string config::KEuSolverPath = "/home/jiry/my-euphony";
const std::string config::KCVC5Path = "/home/jiry/2021A/cvc5";

#ifdef LINUX
const std::string config::KIncreParserPath = "/home/jiry/zyw/IncreLanguage_230511/";
#else
const std::string config::KIncreParserPath = "/home/jiry/zyw/IncreLanguage_230511/";
#endif

const bool config::KIsDefaultSelf = true;


TimeRecorder global::recorder;