//
// Created by pro on 2022/9/18.
//


#include "istool/basic/config.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/autolabel/incre_autolabel.h"
#include "glog/logging.h"

using namespace incre;

int main(int argv, char** argc) {
    std::string path, target;

    if (argv > 1) {
        path = argc[1]; target = argc[2];
    } else {
        path = "/home/jiry/2022A/IncreLanguage//tmp.json";
    }
    auto prog = incre::file2program(path);

    auto labeled_prog = incre::autolabel::labelProgram(prog.get());
    incre::printProgram(labeled_prog, target);
}