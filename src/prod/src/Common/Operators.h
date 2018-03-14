// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "OperationResult.h"

namespace Common {
    class Expression;
    typedef std::shared_ptr<Expression> ExpressionSPtr;

    class Operator
    {
    public:
        Operator(int level, std::wstring const& op)
            : level_(level), op_(op)
        {
        }

        __declspec(property(get=getLevel)) int Level;
        int getLevel() const { return level_; }

        __declspec(property(get=getIsInvalid)) bool IsInvalid;
        bool getIsInvalid() const {return op_.compare(L"Invalid") == 0; }

        __declspec(property(get=getOp)) std::wstring const& Op;
        std::wstring const& getOp() const { return op_; }

        virtual OperationResult Evaluate(std::map<std::wstring, std::wstring> & params, ExpressionSPtr arg1, ExpressionSPtr arg2, bool forPrimary) = 0;

        bool Match(std::wstring const& expression, size_t & start);

        __declspec(property(get=getForPrimary)) bool ForPrimary;
        bool getForPrimary() const {return op_.back() == L'P'; }


    private:
        std::wstring op_;
        int level_;
    };

    typedef std::shared_ptr<Operator> OperatorSPtr;

    class LiteralOperator : public Operator
    {
    public:
        LiteralOperator(int level, std::wstring const& op)
            : Operator(level, op), value_(op)
        {
        }

        virtual OperationResult Evaluate(std::map<std::wstring, std::wstring> & params, ExpressionSPtr arg1, ExpressionSPtr arg2, bool forPrimary);

    private:
        std::wstring value_;
    };

    class ComparisonOperator : public Operator
    {
    public:
        ComparisonOperator(int level, std::wstring const& op)
            : Operator(level, op)
        {
        }

        virtual OperationResult Evaluate(std::map<std::wstring, std::wstring> & params, ExpressionSPtr arg1, ExpressionSPtr arg2, bool forPrimary);
    };

    class BooleanOperator : public Operator
    {
    public:
        BooleanOperator(int level, std::wstring const& op)
            : Operator(level, op)
        {
        }

       virtual OperationResult Evaluate(std::map<std::wstring, std::wstring> & params, ExpressionSPtr arg1, ExpressionSPtr arg2, bool forPrimary);
    };
}
