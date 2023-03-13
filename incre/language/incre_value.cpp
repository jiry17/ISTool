//
// Created by pro on 2022/9/17.
//

#include "istool/incre/language/incre_value.h"
#include "istool/incre/language/incre.h"
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

VFunction::VFunction(const std::string &_name): name(_name) {
}

bool VFunction::equal(Value *value) const {
    auto* va = dynamic_cast<VFunction*>(value);
    if (!va) return false;
    return va->name == name;
}

std::string VFunction::toString() const {
    return name;
}

Data VAbsFunction::run(const Term &param, Context *ctx) {
    auto res = incre::subst(term->content, term->name, param);
    return incre::run(res, ctx);
}
VAbsFunction::VAbsFunction(const Term &__term): VFunction("(" + __term->toString() + ")"), _term(__term) {
    term = dynamic_cast<TmAbs*>(_term.get());
}



/*VFunction::VFunction(const Function &_func): func(_func) {}
bool VFunction::equal(Value *value) const {
    LOG(WARNING) << "Checking equivalence between values of type VFunction";
    return false;
}
std::string VFunction::toString() const {
    // LOG(WARNING) << "Get name for a value of type VFunction";
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
}*/

namespace {
    Data _runOp(const std::string &op_name, int param_num, const std::function<Data(const DataList&)>& sem, const DataList& params) {
        if (params.size() == param_num) return sem(params);
        return Data(std::make_shared<VPartialOpFunction>(op_name, param_num, sem, params));
    }
}

VOpFunction::VOpFunction(const std::string &_op_name, int _param_num, const std::function<Data(const DataList &)> &_sem,
                         const Ty &_type): VFunction(_op_name), param_num(_param_num), sem(_sem), type(_type) {
}
Data VOpFunction::run(const Term &term, Context *ctx) {
    auto* tv = dynamic_cast<TmValue*>(term.get());
    if (!tv) LOG(FATAL) << "Unexpected TermType " << term->toString();
    return _runOp(name, param_num, sem, {tv->data});
}

VPartialOpFunction::VPartialOpFunction(const std::string &_op_name, int _param_num,
                                       const std::function<Data(const DataList&)>& _sem, const DataList &_param_list):
                                       VFunction(_op_name + data::dataList2String(_param_list)), param_num(_param_num),
                                       sem(_sem), param_list(_param_list), op_name(_op_name) {
}
Data VPartialOpFunction::run(const Term &term, Context *ctx) {
    auto* tv = dynamic_cast<TmValue*>(term.get());
    if (!tv) LOG(FATAL) << "Unexpected TermType " << term->toString();
    auto extended_param = param_list; extended_param.push_back(tv->data);
    return _runOp(op_name, param_num, sem, extended_param);
}

Ty incre::getValueType(Value *v) {
    if (dynamic_cast<VUnit*>(v)) return std::make_shared<TyUnit>();
    if (dynamic_cast<VInt*>(v)) return std::make_shared<TyInt>();
    if (dynamic_cast<VBool*>(v)) return std::make_shared<TyBool>();
    auto* vo = dynamic_cast<VOpFunction*>(v);
    if (vo) return vo->type;
    LOG(FATAL) << "User cannot write " << v->toString() << " directly.";
}
