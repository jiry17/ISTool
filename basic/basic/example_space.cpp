//
// Created by pro on 2021/12/4.
//

#include "example_space.h"

bool IOExample::isSatisfy(const PProgram &program) {
    auto* drs = dynamic_cast<DummyRecordSemantics*>(program->semantics.get());
    if (drs) {
#ifdef DEBUG
        assert(program->sub_list.size() == 1);
#endif
        return program::run(program->sub_list[0], inp_list) == oup;
    }
    return program::run(program, inp_list) == oup;
}
IOExample::IOExample(DataList &&_inp_list, Data &&_oup): inp_list(_inp_list), oup(_oup) {
}
std::string IOExample::toString() const {
    return data::dataList2String(inp_list) + " -> " + oup.toString();
}

ExampleSpace::ExampleSpace(const std::vector<PExample> &_example_space): example_space(_example_space) {
}

FiniteExampleSpace::FiniteExampleSpace(const std::vector<PExample> &_example_list): ExampleSpace(_example_list) {
}
PExample FiniteExampleSpace::get(int i) {
    assert(0 <= i && i < example_space.size());
    return example_space[i];
}
int FiniteExampleSpace::size() const {
    return example_space.size();
}

PExample StreamedExampleSpace::get(int i) {
    while (example_space.size() <= i) extendExampleSpace();
    return example_space[i];
}
