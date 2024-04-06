//
// Created by pro on 2021/11/30.
//

#include "istool/basic/data.h"
#include "istool/basic/semantics.h"
#include "istool/incre/language/incre_semantics.h"
#include <cassert>
#include "glog/logging.h"

Data::Data(): value(new NullValue()) {
}

Data::Data(PValue &&_value): value(_value) {
}

std::string Data::toString() const {
    return value->toString();
}

bool Data::operator==(const Data &d) const {
    return value->equal(d.value.get());
}

bool Data::operator < (const Data& d) const {
    return (*this) <= d && !(*this == d);
}

bool Data::operator <= (const Data& d) const {
    auto* cv1 = dynamic_cast<ComparableValue*>(value.get());
    auto* cv2 = dynamic_cast<ComparableValue*>(d.value.get());
    if (!cv1 || !cv2) throw SemanticsError();
    return cv1->leq(d.value.get());
}

Value * Data::get() const {
    return value.get();
}

bool Data::isTrue() const {
    auto* bv = dynamic_cast<BoolValue*>(value.get());
    return bv->w;
}

bool Data::isNull() const {
    auto* nv = dynamic_cast<NullValue*>(value.get());
    return nv;
}

std::string data::dataList2String(const DataList &data_list) {
    if (data_list.empty()) return "[]";
    std::string res;
    for (auto& d: data_list) res += "," + d.toString();
    res[0] = '['; res += ']';
    return res;
}

DataList data::concatDataList(const DataList &x, const DataList &y) {
    auto res = x;
    for (auto& data: y) res.push_back(data);
    return res;
}

namespace {
    void _cartesianProduct(int pos, const DataStorage& separate_data, DataList& cur, DataStorage& res) {
        if (pos == separate_data.size()) {
            res.push_back(cur); return;
        }
        for (auto& d: separate_data[pos]) {
            cur.push_back(d);
            _cartesianProduct(pos + 1, separate_data, cur, res);
            cur.pop_back();
        }
    }
}

DataStorage data::cartesianProduct(const DataStorage &separate_data) {
    DataList cur; DataStorage res;
    _cartesianProduct(0, separate_data, cur, res);
    return res;
}

using namespace ::incre::semantics;

DataList data::data2DataList(const Data& data) {
    DataList result = std::vector<Data>();
    std::shared_ptr<VInd> value_ind = std::static_pointer_cast<VInd>(data.value);
    while (value_ind->name == "Cons") {
        std::shared_ptr<ProductValue> value_product = std::static_pointer_cast<ProductValue>(value_ind->body.value);
        if (value_product->elements.size() != 2) {
            LOG(FATAL) << "elements.size() != 2";
        }
        std::shared_ptr<ProductValue> value_data = std::static_pointer_cast<ProductValue>(value_product->elements[0].value);
        if (!value_data) {
            LOG(FATAL) << "elements[0] is not product value!";
        }
        result.push_back(Data(value_data));
        value_ind = std::static_pointer_cast<VInd>(value_product->elements[1].value);
        if (!value_ind) {
            LOG(FATAL) << "elements[1] is not ind value!";
        }
    }
    return result;
}

int data::getIntFromData(Data& data) {
    std::shared_ptr<IntValue> int_value = std::static_pointer_cast<IntValue>(data.value);
    if (!int_value) {
        LOG(FATAL) << "data.value is not IntValue";
    }
    return int_value->w;
}

bool data::getBoolFromData(Data& data) {
    std::shared_ptr<BoolValue> bool_value = std::static_pointer_cast<BoolValue>(data.value);
    if (!bool_value) {
        LOG(FATAL) << "data.value is not IntValue";
    }
    return bool_value->w;
}