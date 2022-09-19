//
// Created by pro on 2022/9/15.
//

#ifndef ISTOOL_INCRE_TYPE_H
#define ISTOOL_INCRE_TYPE_H

#include <memory>
#include <vector>

namespace incre {
    class TyData {
    public:
        virtual std::string toString() const = 0;
    };

    typedef std::shared_ptr<TyData> Ty;
    typedef std::vector<Ty> TyList;

    class TyInt: public TyData {
    public:
        TyInt() = default;
        virtual ~TyInt() = default;
        virtual std::string toString() const;
    };

    class TyUnit: public TyData {
    public:
        TyUnit() = default;
        virtual ~TyUnit() = default;
        virtual std::string toString() const;
    };

    class TyBool: public TyData {
    public:
        TyBool() = default;
        virtual ~TyBool() = default;
        virtual std::string toString() const;
    };

    class TyTuple: public TyData {
    public:
        TyList fields;
        TyTuple(const TyList& _fields);
        virtual ~TyTuple() = default;
        virtual std::string toString() const;
    };

    class TyVar: public TyData {
    public:
        std::string name;
        TyVar(const std::string& _name);
        virtual ~TyVar() = default;
        virtual std::string toString() const;
    };

    class TyArrow: public TyData {
    public:
        Ty source, target;
        TyArrow(const Ty& _source, const Ty& _target);
        virtual ~TyArrow() = default;
        virtual std::string toString() const;
    };

    class TyInductive: public TyData {
    public:
        std::string name;
        std::vector<std::pair<std::string, Ty>> constructors;
        TyInductive(const std::string& _name, const std::vector<std::pair<std::string, Ty>>& _constructors);
        virtual std::string toString() const;
        virtual ~TyInductive() = default;
    };

    class TyCompress: public TyData {
    public:
        Ty content;
        TyCompress(const Ty& _content);
        virtual std::string toString() const;
        virtual ~TyCompress() =default;
    };

}

#endif //ISTOOL_INCRE_TYPE_H
