//
// Created by pro on 2021/12/9.
//

#include "component_based_solver.h"
#include "basic/ext/z3/z3_util.h"
#include "glog/logging.h"

ComponentBasedSynthesizer::ComponentBasedSynthesizer(Z3GrammarEncoderBuilder *_builder): builder(_builder) {
}

ComponentBasedSynthesizer::~ComponentBasedSynthesizer() noexcept {
    delete builder;
}

PProgram ComponentBasedSynthesizer::synthesis(Specification* spec, const std::vector<PExample>& example_list) {
//    LOG(INFO) << "Synthesis for " << example_list.size() << " examples";
//    for (auto& example: example_list) std::cout << example->toString() << std::endl;
    std::vector<Z3GrammarEncoder*> encoder_list;
    for (auto& syn_info: spec->info_list) {
        auto* encoder = builder->buildEncoder(syn_info->grammar, &ext::z3_ctx);
        encoder_list.push_back(encoder);
    }
    while (1) {
        z3::solver solver(ext::z3_ctx);
        for (int i = 0; i < spec->info_list.size(); ++i) {
            auto& info = spec->info_list[i];
            auto* encoder = encoder_list[i];
            solver.add(z3::mk_and(encoder->encodeStructure(info->name + "@structure")));
        }
        for (int i = 0; i < example_list.size(); ++i) {
            auto &example = example_list[i];
            auto *io_example = dynamic_cast<IOExample *>(example.get());
            if (!io_example) {
                LOG(FATAL) << "Currently support only IO examples";
            }
            auto *encoder = encoder_list[0];
            auto &info = spec->info_list[0];
            z3::expr_vector inp_list(ext::z3_ctx);
            for (auto &inp: io_example->inp_list) {
                inp_list.push_back(ext::buildConst(inp, ext::z3_ctx));
            }
            auto encode_res = encoder->encodeExample(inp_list, info->name + "@val@" + std::to_string(i));
            solver.add(z3::mk_and(encode_res.second));
            solver.add(encode_res.first == ext::buildConst(io_example->oup, ext::z3_ctx));
        }
        auto res = solver.check();
        if (res == z3::unsat) {
            for (auto* encoder: encoder_list) {
                encoder->enlarge();
            }
        } else {
            auto model = solver.get_model();
            auto res = encoder_list[0]->programBuilder(model);
            for (auto* encoder: encoder_list) delete encoder;
            LOG(INFO) << "res " << res->toString() << std::endl;
            return res;
        }
    }

}