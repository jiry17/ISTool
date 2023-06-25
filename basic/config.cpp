//
// Created by pro on 2021/12/5.
//

#include "istool/basic/config.h"

#ifdef LINUX
const std::string config::KSourcePath = "/home/jiry/2023S/ISTool/";
#else
const std::string config::KSourcePath = "/Users/pro/Desktop/work/2023S/ISTool/";
#endif
const std::string config::KEuSolverPath = "/home/jiry/my-euphony";
const std::string config::KCVC5Path = "/home/jiry/2021A/cvc5";

#ifdef LINUX
const std::string config::KIncreParserPath = "/home/jiry/2023S/IncreLanguage/";
#else
const std::string config::KIncreParserPath = "/Users/pro/Desktop/work/2023S/IncreLanguage/";
#endif

const bool config::KIsDefaultSelf = true;


TimeRecorder global::recorder;