//
// Created by pro on 2022/1/15.
//

#include "istool/ext/deepcoder/data_type.h"
#include "istool/sygus/theory/basic/clia/clia_type.h"

SumType::SumType(const TypeList &_sub_types): sub_types(_sub_types) {
}
std::string SumType::getName() {
    std::string res;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (i) res += "+";
        res += sub_types[i]->getName();
    }
    return res;
}
bool SumType::equal(Type *type) {
    auto* st = dynamic_cast<SumType*>(type);
    if (!st) return false;
    if (st->sub_types.size() != sub_types.size()) return false;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (!sub_types[i]->equal(st->sub_types[i].get())) return false;
    }
    return true;
}

ProductType::ProductType(const TypeList &_sub_types): sub_types(_sub_types) {
}
std::string ProductType::getName() {
    std::string res;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (i) res += "*";
        res += sub_types[i]->getName();
    }
    return res;
}
bool ProductType::equal(Type *type) {
    auto* pt = dynamic_cast<ProductType*>(type);
    if (!pt) return false;
    if (pt->sub_types.size() != sub_types.size()) return false;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (!sub_types[i]->equal(pt->sub_types[i].get())) return false;
    }
    return true;
}

ArrowType::ArrowType(const TypeList &_inp_types, const PType &_oup_type): inp_types(_inp_types), oup_type(_oup_type) {
}
std::string ArrowType::getName() {
    return type::typeList2String(inp_types) + "->" + oup_type->getName();
}
bool ArrowType::equal(Type *type) {
    auto* at = dynamic_cast<ArrowType*>(type);
    if (!at || at->inp_types.size() != inp_types.size()) return false;
    if (!oup_type->equal(at->oup_type.get())) return false;
    for (int i = 0; i < inp_types.size(); ++i) {
        if (!inp_types[i]->equal(at->inp_types[i].get())) return false;
    }
    return true;
}

ListType::ListType(const PType &_content): content(_content) {
}
std::string ListType::getName() {
    return "List[" + content->getName() + "]";
}
bool ListType::equal(Type *type) {
    auto* lt = dynamic_cast<ListType*>(type);
    if (!lt) return false;
    return content->equal(lt->content.get());
}

BTreeType::BTreeType(const PType &_content, const PType &_leaf): content(_content), leaf(_leaf) {
}
std::string BTreeType::getName() {
    return "BTree[" + content->getName() + "," + leaf->getName() + "]";
}
bool BTreeType::equal(Type* type) {
    auto* bt = dynamic_cast<BTreeType*>(type);
    if (!bt) return false;
    return content->equal(bt->content.get()) && leaf->equal(bt->leaf.get());
}

PType ext::ho::getTIntList() {
    static PType res;
    if (!res) res = std::make_shared<ListType>(theory::clia::getTInt());
    return res;
}

PType ext::ho::getTVarAList() {
    static PType res;
    if (!res) res = std::make_shared<ListType>(type::getTVarA());
    return res;
}