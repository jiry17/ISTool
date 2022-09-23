//
// Created by pro on 2022/9/17.
//

#include "istool/incre/language/incre_value.h"
#include "glog/logging.h"

using namespace incre;

std::string VUnit::toString() const {
    return "unit";
}
bool VUnit::equal(Value *value) const {
    return dynamic_cast<VUnit*>(value);
}

VInductive::VInductive(const std::string &_name, const Data &_content): name(_name), content(_content) {
}
bool VInductive::equal(Value *value) const {
    auto* v = dynamic_cast<VInductive*>(value);
    if (!v) return false;
    return v->name == name && v->content == content;
}
std::string VInductive::toString() const {
    return name + " " + content.toString();
}

VCompress::VCompress(const Data &_content): content(_content) {}
std::string VCompress::toString() const {
    return "compress " + content.toString();
}
bool VCompress::equal(Value *value) const {
    auto* cv = dynamic_cast<VCompress*>(value);
    if (!cv) return false;
    return content == cv->content;
}

VFunction::VFunction(const Function &_func): func(_func) {}
bool VFunction::equal(Value *value) const {
    LOG(WARNING) << "Checking equivalence between values of type VFunction";
    return false;
}
std::string VFunction::toString() const {
    LOG(WARNING) << "Get name for a value of type VFunction";
    return "function";
}

VNamedFunction::VNamedFunction(const Function &_func, const std::string &_name):
    VFunction(_func), name(_name) {
}
bool VNamedFunction::equal(Value *value) const {
    auto* vnf = dynamic_cast<VNamedFunction*>(value);
    return vnf && vnf->name == name;
}
std::string VNamedFunction::toString() const {
    return name;
}

VTyped::VTyped(const Ty &_type): type(_type) {}
VBasicOperator::VBasicOperator(const Function &_func, const std::string &_name, const Ty &_type):
    VNamedFunction(_func, _name), VTyped(_type) {
}
