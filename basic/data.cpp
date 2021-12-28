//
// Created by pro on 2021/11/30.
//

#include "istool/basic/data.h"
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

bool Data::operator <= (const Data& d) const {
    auto* cv1 = dynamic_cast<ComparableValue*>(value.get());
    auto* cv2 = dynamic_cast<ComparableValue*>(d.value.get());
#ifdef DEBUG
    if (!cv1 || !cv2) assert(0);
#endif
    return cv1->leq(d.value.get());
}

Value * Data::get() const {
    return value.get();
}

PType Data::getPType() const {
    auto* tv = dynamic_cast<TypedValue*>(value.get());
    if (!tv) {
        LOG(FATAL) << "Cannot get type for data " << toString();
    }
    return tv->type;
}

Type * Data::getType() const {
    return getPType().get();
}

bool Data::isTrue() const {
    auto* bv = dynamic_cast<BoolValue*>(value.get());
    return bv->w;
}

std::string data::dataList2String(const DataList &data_list) {
    std::string res;
    for (auto& d: data_list) res += "," + d.toString();
    res[0] = '['; res += ']';
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