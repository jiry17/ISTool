//
// Created by pro on 2021/12/31.
//

#include "istool/sygus/theory/witness/string/string_witness.h"
#include "istool/sygus/theory/witness/clia/clia_wit_value.h"
#include "istool/sygus/theory/basic/string/str.h"
#include "istool/sygus/theory/basic/clia/clia.h"
#include "glog/logging.h"

using theory::string::getStringValue;
using theory::clia::getIntValue;

namespace {
    std::string _getString(const WitnessData& d) {
        auto* dv = dynamic_cast<DirectWitnessValue*>(d.get());
        if (!dv) WitnessError(d);
        auto* sv = dynamic_cast<StringValue*>(d.get());
        if (!sv) WitnessError(d);
        return sv->s;
    }
}

WitnessList StringCatWitnessFunction::witness(const WitnessData &oup) {
    if (dynamic_cast<TotalWitnessValue*>(oup.get())) return {{oup, oup}};
    auto s = _getString(oup); int n = s.length();
    WitnessList res;
    for (int i = 0; i <= n; ++i) {
        auto prefix = s.substr(0, i), suffix = s.substr(i, n);
        res.push_back({BuildDirectWData(String, prefix), BuildDirectWData(String, suffix)});
    }
    return res;
}

namespace {
    bool _isValidReplace(std::string inp, const std::string& s, const std::string &t, const std::string& oup) {
        auto pos = inp.find(s);
        if (pos == std::string::npos) return inp == oup;
        return inp.replace(pos, s.length(), t) == oup;
    }

    void _collectAllValidReplace(const std::string& s, const std::string& t, const std::string& oup, WitnessList& res) {
        auto pos = oup.find(t);
        while (pos != std::string::npos) {
            auto inp = oup; inp.replace(pos, t.length(), s);
            if (_isValidReplace(inp, s, t, oup)) {
                res.push_back({BuildDirectWData(String, inp), BuildDirectWData(String, s), BuildDirectWData(String, oup)});
            }
        }
    }

    std::vector<std::string> _getStringConstList(DataList* const_list) {
        std::vector<std::string> res;
        for (const auto& d: *const_list) {
            res.push_back(getStringValue(d));
        }
        return res;
    }
}

WitnessList StringReplaceWitnessFunction::witness(const WitnessData &oup) {
    if (dynamic_cast<TotalWitnessValue*>(oup.get())) return {{oup, oup, oup}};
    std::vector<std::string> s_list = _getStringConstList(const_list);
    auto oup_s = _getString(oup);
    WitnessList res;

    // both s and t are not empty
    for (const auto& s: s_list) {
        for (const auto& t: s_list) {
            if (s == t) continue;
            _collectAllValidReplace(s, t, oup_s, res);
        }
    }
    // s = "" is duplicated with str.++, ignore
    // t = ""
    int n = oup_s.length();
    for (const auto& t: s_list) {
        for (int i = 0; i < oup_s.length(); ++i) {
            auto inp = oup_s.substr(0, i) + t + oup_s.substr(i, n);
            if (_isValidReplace(inp, "", t, oup_s)) {
                res.push_back({BuildDirectWData(String, inp), BuildDirectWData(String, ""), BuildDirectWData(String, t)});
            }
        }
    }
    return res;
}

WitnessList StringAtWitnessFunction::witness(const WitnessData &oup) {
    if (dynamic_cast<TotalWitnessValue*>(oup.get())) return {{oup, oup}};
    auto oup_s = _getString(oup);
    if (oup_s.length() > 0) return {};
    auto s_list = _getStringConstList(const_list);
    WitnessList res;
    for (auto& inp: s_list) {
        for (int i = 0; i < inp.length(); ++i) {
            if (inp[i] == oup_s[0]) {
                res.push_back({BuildDirectWData(String, inp), BuildDirectWData(Int, i)});
            }
        }
    }
    return res;
}

WitnessList IntToStrWitnessFunction::witness(const WitnessData &oup) {
    if (dynamic_cast<TotalWitnessValue*>(oup.get())) return {{oup}};
    auto oup_s = _getString(oup);
    int inf_value = getIntValue(*inf);
    for (char c: oup_s) if (c < '0' || c > '9') return {};
    int res = 0;
    for (char c: oup_s) {
        long long ne = 10ll * res + int(c) - int('0');
        if (ne > inf_value) return {};
        res = int(ne);
    }
    return {{BuildDirectWData(Int, res)}};
}

StringSubStrWitnessFunction::StringSubStrWitnessFunction(DataList *_const_list, Data *_int_min, Data *_int_max):
    const_list(_const_list), int_min(_int_min), int_max(_int_max) {
}

void _collectAllValidSubstr(const std::string& s, const std::string& t, WitnessList& res, int l, int r) {
    if (s.length() > t.length()) return;
    if (t.empty()) {
        auto total = std::make_shared<TotalWitnessValue>();
        if (l <= -1) {
            res.push_back({BuildDirectWData(String, s), theory::clia::buildRange(l, -1), total});
        }
        if (l <= 0) {
            res.push_back({BuildDirectWData(String, s), total, theory::clia::buildRange(l, 0)});
        }
        if (s.length() <= r) {
            res.push_back({BuildDirectWData(String, s), theory::clia::buildRange(s.length(), r), total});
        }
    } else {
        int n = s.length(), m = t.length();
        for (auto pos = s.find(t); pos != std::string::npos; pos = s.find(t, pos + 1)) {
            if (pos != n - m) {
                res.push_back({BuildDirectWData(String, s), BuildDirectWData(Int, pos), BuildDirectWData(Int, m)});
            } else if (m <= r) {
                res.push_back({BuildDirectWData(String, s), BuildDirectWData(Int, pos), theory::clia::buildRange(m, r)});
            }
        }
    }
}

WitnessList StringSubStrWitnessFunction::witness(const WitnessData &oup) {
    if (dynamic_cast<TotalWitnessValue*>(oup.get())) return {{oup, oup, oup}};
    auto s_list = _getStringConstList(const_list);
    auto oup_s = _getString(oup);
    auto l = getIntValue(*int_min), r = getIntValue(*int_max);
    WitnessList res;
    for (const auto& s: s_list) {
        _collectAllValidSubstr(s, oup_s, res, l, r);
    }
    return res;
}