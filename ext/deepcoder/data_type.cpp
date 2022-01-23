//
// Created by pro on 2022/1/15.
//

#include "istool/ext/deepcoder/data_type.h"
#include "istool/ext/deepcoder/data_value.h"
#include "istool/sygus/theory/basic/clia/clia_type.h"

TSum::TSum(const TypeList &_sub_types): sub_types(_sub_types) {
}
std::string TSum::getName() {
    std::string res;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (i) res += "+";
        res += sub_types[i]->getName();
    }
    return res;
}
bool TSum::equal(Type *type) {
    auto* st = dynamic_cast<TSum*>(type);
    if (!st) return false;
    if (st->sub_types.size() != sub_types.size()) return false;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (!sub_types[i]->equal(st->sub_types[i].get())) return false;
    }
    return true;
}

TProduct::TProduct(const TypeList &_sub_types): sub_types(_sub_types) {
}
std::string TProduct::getName() {
    std::string res;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (i) res += "*";
        res += sub_types[i]->getName();
    }
    return res;
}
bool TProduct::equal(Type *type) {
    auto* pt = dynamic_cast<TProduct*>(type);
    if (!pt) return false;
    if (pt->sub_types.size() != sub_types.size()) return false;
    for (int i = 0; i < sub_types.size(); ++i) {
        if (!sub_types[i]->equal(pt->sub_types[i].get())) return false;
    }
    return true;
}

TArrow::TArrow(const TypeList &_inp_types, const PType &_oup_type): inp_types(_inp_types), oup_type(_oup_type) {
}
std::string TArrow::getName() {
    return type::typeList2String(inp_types) + "->" + oup_type->getName();
}
bool TArrow::equal(Type *type) {
    auto* at = dynamic_cast<TArrow*>(type);
    if (!at || at->inp_types.size() != inp_types.size()) return false;
    if (!oup_type->equal(at->oup_type.get())) return false;
    for (int i = 0; i < inp_types.size(); ++i) {
        if (!inp_types[i]->equal(at->inp_types[i].get())) return false;
    }
    return true;
}

TList::TList(const PType &_content): content(_content) {
}
std::string TList::getName() {
    return "List[" + content->getName() + "]";
}
bool TList::equal(Type *type) {
    auto* lt = dynamic_cast<TList*>(type);
    if (!lt) return false;
    return content->equal(lt->content.get());
}

TBTree::TBTree(const PType &_content, const PType &_leaf): content(_content), leaf(_leaf) {
}
std::string TBTree::getName() {
    return "BTree[" + content->getName() + "," + leaf->getName() + "]";
}
bool TBTree::equal(Type* type) {
    auto* bt = dynamic_cast<TBTree*>(type);
    if (!bt) return false;
    return content->equal(bt->content.get()) && leaf->equal(bt->leaf.get());
}

PType ext::ho::getTIntList() {
    static PType res;
    if (!res) res = std::make_shared<TList>(theory::clia::getTInt());
    return res;
}

PType ext::ho::getTVarAList() {
    static PType res;
    if (!res) res = std::make_shared<TList>(type::getTVarA());
    return res;
}