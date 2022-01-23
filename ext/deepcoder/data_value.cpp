//
// Created by pro on 2022/1/15.
//

#include "istool/ext/deepcoder/data_type.h"
#include "istool/ext/deepcoder/data_value.h"
#include "glog/logging.h"

ProductValue::ProductValue(const DataList &_elements): elements(_elements) {
}
Data ProductValue::get(int id) const {
    if (id < 0 || id > elements.size()) {
        LOG(FATAL) << "Access product " << toString() << " with invalid index " << id;
    }
    return elements[id];
}
std::string ProductValue::toString() const {
    std::string res = "(";
    for (int i = 0; i < elements.size(); ++i) {
        if (i) res += ","; res += elements[i].toString();
    }
    return res + ")";
}
bool ProductValue::equal(Value *v) const {
    auto* pv = dynamic_cast<ProductValue*>(v);
    if (!pv || pv->elements.size() != elements.size()) return false;
    for (int i = 0; i < elements.size(); ++i) {
        if (!(elements[i] == pv->elements[i])) return false;
    }
    return true;
}

SumValue::SumValue(int _id, const Data &_value): id(_id), value(_value) {}
std::string SumValue::toString() const {
    return std::to_string(id) + "@" + value.toString();
}
bool SumValue::equal(Value *v) const {
    auto* sv = dynamic_cast<SumValue*>(v);
    if (!sv) return false;
    return sv->id == id && sv->value == value;
}

ListValue::ListValue(const DataList &_value): value(_value) {}
std::string ListValue::toString() const {
    return data::dataList2String(value);
}
bool ListValue::equal(Value *v) const {
    auto* dv = dynamic_cast<ListValue*>(v);
    if (!dv) return false;
    if (dv->value.size() != value.size()) return false;
    for (int i = 0; i < value.size(); ++i)
        if (!(value[i] == dv->value[i])) return false;
    return true;
}

BTreeInternalValue::BTreeInternalValue(const BTreeNode &_l, const BTreeNode &_r, const Data &_v): l(_l), r(_r), value(_v) {}
std::string BTreeInternalValue::toString() const {
    return "(" + l->toString() + "," + value.toString() + "," + r->toString() + ")";
}
bool BTreeInternalValue::equal(Value *v) const {
    auto* iv = dynamic_cast<BTreeInternalValue*>(v);
    if (!iv) return false;
    return iv->value == value && iv->l->equal(l.get()) && iv->r->equal(r.get());
}

BTreeLeafValue::BTreeLeafValue(const Data &_v): value(_v) {}
std::string BTreeLeafValue::toString() const {
    return value.toString();
}
bool BTreeLeafValue::equal(Value *v) const {
    auto* lv = dynamic_cast<BTreeLeafValue*>(v);
    if (!lv) return false;
    return lv->value == value;
}

Data ext::ho::buildList(const DataList &content) {
    return Data(std::make_shared<ListValue>(content));
}
Data ext::ho::buildProduct(const DataList &elements) {
    return Data(std::make_shared<ProductValue>(elements));
}
Data ext::ho::buildSum(int id, const Data &value) {
    return Data(std::make_shared<SumValue>(id, value));
}