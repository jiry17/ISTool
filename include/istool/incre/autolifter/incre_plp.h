//
// Created by pro on 2022/9/27.
//

#ifndef ISTOOL_INCRE_PLP_H
#define ISTOOL_INCRE_PLP_H

#include "incre_autolifter_solver.h"
#include "istool/basic/grammar.h"

namespace incre::autolifter {
    class PLPTask {
    public:
        FExampleSpace* example_space;
        std::vector<Grammar*> f_grammar_list;
        Grammar* const_grammar;
        TypedProgram target;
        std::vector<int> path;
        int target_compress_id;


        DataList runInp(int example_id, int id, Program* program);
        Data runOup(int example_id);
        IOExample getIO(int example_id, const std::vector<std::pair<int, PProgram>>& aux_list);
        int acquireExample(int target_num, int timeout);
        PLPTask(FExampleSpace* _example_space, const std::vector<Grammar*>& _f_grammar_list, Grammar* _const_grammar,
                const TypedProgram& _target, const std::vector<int>& _path, int target_compress_id);
    };

    Data eliminateCompress(const Data& data);
    Data openLabeledCompress(const Data& data, int label);
}

#endif //ISTOOL_INCRE_PLP_H
