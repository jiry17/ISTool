//
// Created by zyw on 2024/3/29.
//

#include "istool/basic/data.h"
#include "istool/basic/config.h"
#include "istool/basic/grammar.h"
#include "istool/basic/time_guard.h"
#include "istool/incre/analysis/incre_instru_info.h"
#include "istool/incre/analysis/incre_instru_runtime.h"
#include "istool/incre/autolifter/incre_autolifter_solver.h"
#include "istool/incre/language/incre_dp.h"
#include "istool/incre/grammar/incre_grammar_semantics.h"
#include "istool/incre/io/incre_from_json.h"
#include "istool/incre/io/incre_printer.h"
#include "istool/incre/trans/incre_trans.h"
#include <iostream>
#include <fstream>
#include <tuple>
#include "glog/logging.h"

 using namespace incre;
using namespace std::literals;
#define Print(x) std::cout << #x " = " << x << std::endl;
#define Print_n(n) for (int i = 0; i < n; ++i) std::cout << std::endl;

DEFINE_string(sufu_input, "/home/jiry/zyw/dp/IncreLanguage-DSL/dp-benchmark/maximal_problem-input/", "The absolute path of the sufu input folder");
DEFINE_string(sufu_output, "/home/jiry/zyw/dp/IncreLanguage-DSL/dp-benchmark/maximal_problem-sufu_result/", "The absolute path of the sufu output folder");
DEFINE_string(dp_output, "/home/jiry/zyw/dp/IncreLanguage-DSL/dp-benchmark/maximal_problem-dp/", "The absolute path of the dp synthesis output folder");
// DEFINE_string(sufu_input, "/home/jiry/zyw/dp/IncreLanguage-DSL/dp-benchmark/satisfy_problem-input/", "The absolute path of the sufu input folder");
// DEFINE_string(sufu_output, "/home/jiry/zyw/dp/IncreLanguage-DSL/dp-benchmark/satisfy_problem-sufu_result/", "The absolute path of the sufu output folder");
// DEFINE_string(dp_output, "/home/jiry/zyw/dp/IncreLanguage-DSL/dp-benchmark/satisfy_problem-dp/", "The absolute path of the dp synthesis output folder");
DEFINE_string(file_name, "198.f", "The file name of the dp synthesis benchmark");
DEFINE_bool(autolabel, false, "Whether automatically generate annotations");
DEFINE_bool(mark_rewrite, false, "Whether to mark the sketch holes.");

// config for building grammar
int sample_size = 3;
int single_example_num = 100;

int main(int argv, char** argc) {
    gflags::ParseCommandLineFlags(&argv, &argc, true);

    std::string sufu_input_path = FLAGS_sufu_input + FLAGS_file_name;
    std::string sufu_output_path = FLAGS_sufu_output + FLAGS_file_name;
    std::string dp_output_path = FLAGS_dp_output + FLAGS_file_name;

    IncreProgram sufu_input_prog = io::parseFromF(sufu_input_path);
    printProgram(sufu_input_prog);

    auto env = std::make_shared<Env>();
    incre::config::applyConfig(sufu_input_prog.get(), env.get());

    auto sufu_input_ctx = buildContext(sufu_input_prog.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
    auto sufu_input_info = incre::analysis::buildIncreInfo(sufu_input_prog.get(), env.get());

    IncreProgram sufu_output_prog = io::parseFromF(sufu_output_path);
    printProgram(sufu_output_prog);

    auto sufu_output_ctx = buildContext(sufu_output_prog.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
    auto sufu_output_info = incre::analysis::buildIncreInfo(sufu_output_prog.get(), env.get());
    // change SampleSize
    sufu_output_info->example_pool->changeKSizeLimit(sample_size);

    // used in evaluation
    FunctionContext func_ctx;
    incre::semantics::DefaultEvaluator default_eval = incre::semantics::DefaultEvaluator();

    for (int i = 0; i < single_example_num; ++i) {
        std::pair<syntax::Term, DataList> tmp = sufu_output_info->example_pool->generateStart();
        syntax::Term example_term = tmp.first;
        Print(example_term->toString());
        // Print(incre::syntax::termType2String(example_term->getType()));
        // auto sufu_output_result = default_eval.evaluate(example_term.get(), sufu_output_ctx->ctx);
        // Print(sufu_output_result.value->toString());
        // Print_n(2);
    }

    // // test evaluation
    // std::shared_ptr<Value> a = std::make_shared<IntValue>(1);
    // Data param_data;
    // param_data.value = a;
    // incre::syntax::Term param = std::make_shared<incre::syntax::TmValue>(param_data);
    // incre::syntax::Term func = std::make_shared<incre::syntax::TmVar>("test_id");
    // incre::syntax::Term test_term = std::make_shared<incre::syntax::TmApp>(func, param);
    // auto test_result = default_eval.evaluate(test_term.get(), sufu_output_ctx->ctx);
    // Print(test_result.toString());
    
}