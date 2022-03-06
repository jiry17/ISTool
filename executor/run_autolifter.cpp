//
// Created by pro on 2022/2/23.
//

#include "istool/dsl/autolifter/autolifter_dataset.h"
#include "istool/invoker/lifting_invoker.h"

int main(int argv, char** argc) {
    auto* task = dsl::autolifter::getLiftingTask("lazy-tag", "sum@neg");
    auto res = invoker::single::invokeAutoLifter(task, nullptr, {});
    std::cout << res.toString() << std::endl;
    return 0;
}