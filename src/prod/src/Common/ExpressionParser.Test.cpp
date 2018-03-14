// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Expression.h"
#include "Operators.h"

using namespace std;

namespace Common
{
    class ExpressionParserTest
    {
    };

    BOOST_FIXTURE_TEST_SUITE(ExpressionParserTestSuite,ExpressionParserTest)

    BOOST_AUTO_TEST_CASE(EmptyString)
    {
        wstring error;
        map<wstring, wstring> params;
        params.insert(make_pair<wstring, wstring>(L"A", L"5"));

        ExpressionSPtr compiledExpression = Expression::Build(L"");
        bool result = compiledExpression->Evaluate(params, error);
        VERIFY_IS_TRUE(result);
        VERIFY_IS_TRUE(error == L"");
    }

    BOOST_AUTO_TEST_CASE(ComparisonOperatorTest)
    {
        ExpressionSPtr compiledExpression = Expression::Build(L"a == 5");
        map<wstring, wstring> params;
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        OperationResult result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        wstring error;
        bool boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        params.clear();
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a == alpha");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"alpha"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"beta"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a != 5");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a != alpha");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"alpha"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"beta"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a >= 5");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"4"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a > 5");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a < 5");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"4"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a <= 5");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"!(a > 5)");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"3"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        // operator ^ for string
        compiledExpression = Expression::Build(L"a ^ Str");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"String"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"Str"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"tring"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"Str"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        // operator ^ for Uri
        compiledExpression = Expression::Build(L"FaultDomain ^ fd:/DC0");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC0/Rack0/Shelf0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC0/Rack0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC00/Rack0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        // operator !^ for Uri
        compiledExpression = Expression::Build(L"FaultDomain !^ fd:/DC0");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC0/Rack0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC1"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        // operator ^P for Uri
        compiledExpression = Expression::Build(L"FaultDomain ^P fd:/DC0");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC0/Rack0"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"fd:/DC00/Rack0"));
        result = compiledExpression->Evaluate(params);
        // the result is true because ^P is skipped when evaluate for regular (non-primary) value
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        result = compiledExpression->Evaluate(params, true);
        // the result is false when evaluate for primary value
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        // numberic inputs are invalid for ^ 
        compiledExpression = Expression::Build(L"FaultDomain ^ 5");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"FaultDomain", L"55"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        // operator ~ for FDPolicy
        compiledExpression = Expression::Build(L"FaultDomain ^P fd:/DC0 && FDPolicy ~ Nonpacking");
        VERIFY_IS_TRUE(compiledExpression->FDPolicy == L"Nonpacking");

        compiledExpression = Expression::Build(L"FDPolicy ~ Ignore && a == 5 ");
        VERIFY_IS_TRUE(compiledExpression->FDPolicy == L"Ignore");

        compiledExpression = Expression::Build(L"PlacePolicy ~ NonPartially ");
        VERIFY_IS_TRUE(compiledExpression->PlacePolicy == L"NonPartially");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params, error);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);
    }

    BOOST_AUTO_TEST_CASE(BooleanOperatorTest)
    {
        ExpressionSPtr compiledExpression = Expression::Build(L"a == 5 && b != 6");
        map<wstring, wstring> params;
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        params.insert(make_pair<wstring, wstring>(L"b", L"5"));
        OperationResult result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        params.insert(make_pair<wstring, wstring>(L"b", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        wstring error;
        bool boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"a == 5 || b != 6");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        params.insert(make_pair<wstring, wstring>(L"b", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        params.insert(make_pair<wstring, wstring>(L"b", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);

        params.clear();
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"6"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"(a == 5) == (b == 5)");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"b", L"5"));
        error = L"";
        boolResult = compiledExpression->Evaluate(params, error);
        VERIFY_ARE_NOT_EQUAL(L"", error);
        VERIFY_IS_FALSE(boolResult);

        compiledExpression = Expression::Build(L"! (a == 5)");

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"6"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_TRUE(result.NonLiteral);

        params.clear();
        params.insert(make_pair<wstring, wstring>(L"a", L"5"));
        result = compiledExpression->Evaluate(params);
        VERIFY_IS_TRUE(result.IsNonLiteral());
        VERIFY_IS_FALSE(result.NonLiteral);
    }

    BOOST_AUTO_TEST_CASE(InvalidExpressionTest)
    {
        VERIFY_IS_TRUE(Expression::Build(L"(abc") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"abc)") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"1==") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"==1") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"==") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"!") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"!a && !") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"a!Param)") == nullptr);

        VERIFY_IS_TRUE(Expression::Build(L"()var") == nullptr);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
