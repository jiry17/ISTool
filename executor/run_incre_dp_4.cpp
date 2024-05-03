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
int sample_size = 20;
int single_example_num = 100;

int main(int argv, char** argc) {
    gflags::ParseCommandLineFlags(&argv, &argc, true);

    std::string sufu_input_path = FLAGS_sufu_input + FLAGS_file_name;
    std::string sufu_output_path = FLAGS_sufu_output + FLAGS_file_name;
    std::string dp_output_path = FLAGS_dp_output + FLAGS_file_name;
    bool is_autolabel = FLAGS_autolabel;
    bool is_highlight_replace = FLAGS_mark_rewrite;

    global::recorder.start("all");
    global::recorder.start("1-SuFu");
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
    global::recorder.end("1-SuFu");

    global::recorder.start("2-get type of partial solution");
    // get the type of partial solution
    incre::syntax::Ty solution_ty = incre::syntax::getSolutionType(sufu_output_prog.get(), sufu_output_ctx);
    std::cout << "zyw: solution_ty = ";
    if (solution_ty) {
        std::cout << solution_ty->toString() << std::endl;
    } else {
        LOG(FATAL) << "solution ty not found!";
    }
    // translate Ty to Type
    PType solution_type = incre::trans::typeFromIncre(solution_ty.get());
    std::cout << "zyw: solution_type = " << solution_type->getName() << std::endl;
    // get solution length
    int solution_len;
    if (solution_ty->getType() == incre::syntax::TypeType::INT) {
        solution_len = 1;
    } else if (solution_ty->getType() == incre::syntax::TypeType::TUPLE) {
        std::shared_ptr<incre::syntax::TyTuple> sol_tuple = std::static_pointer_cast<incre::syntax::TyTuple>(solution_ty);
        solution_len = sol_tuple->fields.size();
        if (solution_len <= 1) {
            LOG(FATAL) << "incorrect solution length";
        }
    } else {
        LOG(FATAL) << "unknown solution type";
    }
    Print_n(2);
    global::recorder.end("2-get type of partial solution");

    global::recorder.start("3-get all program");
    std::vector<IncreProgram> program_list;
    std::vector<IncreFullContext> program_ctx;
    std::vector<std::string> r_name_list;
    int program_list_len;

    std::string r_filter_name = "r_filter_"s + std::to_string(solution_len);
    for (int i = 1; i <= solution_len; ++i) {
        for (int j = 1; j <= 2; ++j) {
            std::string r_name = "r_"s + std::to_string(solution_len) + "_"s + std::to_string(i) + "_"s + std::to_string(j);
            r_name_list.push_back(r_name);
            // get new program
            IncreProgram new_program = incre::syntax::addRFilter(sufu_output_prog.get(), sufu_output_ctx, r_filter_name, r_name);
            program_list.push_back(new_program);
            printProgram(new_program);
            // get new ctx for this program
            IncreFullContext new_ctx = buildContext(new_program.get(), BuildGen(incre::semantics::DefaultEvaluator), BuildGen(types::DefaultIncreTypeChecker));
            program_ctx.push_back(new_ctx);
        }
    }
    program_list_len = program_list.size();

    global::recorder.end("3-get all program");
    
    global::recorder.start("4-filter program");
    // used in evaluation
    FunctionContext func_ctx;
    incre::semantics::DefaultEvaluator default_eval = incre::semantics::DefaultEvaluator();
    // program result
    std::vector<bool> program_result(program_list_len, true);

    for (int i = 0; i < single_example_num; ++i) {
        std::cout << "zyw: start of cycle " << i << std::endl;
        TimeRecorder partial_recorder;
        partial_recorder.start("cycle time");
        
        syntax::Term example_term = sufu_output_info->example_pool->generateSingleExample_2();
        Print(example_term->toString());
        auto sufu_output_result = default_eval.evaluate(example_term.get(), sufu_output_ctx->ctx);
        Print(sufu_output_result.toString());

        for (int j = 0; j < program_list_len; ++j) {
            if (!program_result[j]) continue;
            auto program_result_j = default_eval.evaluate(example_term.get(), program_ctx[j]->ctx);
            Print(program_result_j.toString());
            if (sufu_output_result.toString() != program_result_j.toString()) {
                program_result[j] = false;
            }
        }

        partial_recorder.end("cycle time");
        partial_recorder.printAll();
        Print_n(2);
    }
    global::recorder.end("4-filter program");

    for (int i = 0; i < program_list_len; ++i) {
        Print(r_name_list[i]);
        Print(program_result[i]);
    }
    Print_n(2);


    // print result
    global::recorder.end("all");
    global::recorder.printAll();



    // print to dp_output_path
    std::ofstream dpOutFile(dp_output_path);
    std::streambuf *dp_cout_buf = std::cout.rdbuf();
    std::cout.rdbuf(dpOutFile.rdbuf());
    

    global::recorder.printAll();

    std::cout.rdbuf(dp_cout_buf);
    dpOutFile.close();

}