//
// Created by pro on 2021/12/5.
//

#include "istool/basic/config.h"
#include "glog/logging.h"

#ifdef LINUX
// const std::string config::KSourcePath = "/home/jiry/zyw/dp/ISTool/";
const std::string config::KSourcePath = "/home/jiry/2023A/ISTool/";
#else
const std::string config::KSourcePath = "/Users/pro/Desktop/work/2024S/ISTool/";
#endif
const std::string config::KEuSolverPath = "/home/jiry/my-euphony";
const std::string config::KCVC5Path = "/home/jiry/2021A/cvc5";

#ifdef LINUX
const std::string config::KIncreParserPath = "/usr/jiry/IncreLanguage/";
// const std::string config::KIncreParserPath = "/home/jiry/zyw/dp/IncreLanguage/";
#else
const std::string config::KIncreParserPath = "/Users/pro/Desktop/work/2024S/IncreLanguage/";
#endif

const bool config::KIsDefaultSelf = true;

std::string global::KStageInfoPath = "";
TimeRecorder global::recorder;

void global::printStageResult(const std::string& info) {
    if (KStageInfoPath.empty()) {
        LOG(INFO) << info;
    } else {
        auto* f = std::fopen(KStageInfoPath.c_str(), "a");
        fprintf(f, "%s\n", info.c_str());
        fclose(f);
    }
}