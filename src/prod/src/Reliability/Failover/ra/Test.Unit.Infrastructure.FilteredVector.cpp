// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

struct Functor
{
    bool operator()(int element) { return (element % 2) == 0; }
};

typedef Infrastructure::FilteredVector<int, Functor> FilteredVectorType;

class TestFilteredVector
{
protected:    
    void RunAndVerify(initializer_list<int> initialE, initializer_list<int> expectedE)
    {
        vector<int> initial(initialE.begin(), initialE.end());
        vector<int> expected(expectedE.begin(), expectedE.end());

        Functor f;
        FilteredVectorType obj(initial, f);

        vector<int> actual;
        for (auto it : obj)
        {
            actual.push_back(it);
        }

        Verify::VectorStrict(expected, actual);
    }

    void ClearAndVerify(initializer_list<int> initialE, initializer_list<int> finalE)
    {
        vector<int> initial(initialE.begin(), initialE.end());
        vector<int> expected(finalE.begin(), finalE.end());

        Functor f;
        FilteredVectorType obj(initial, f);

        obj.Clear();

        vector<int> filtered;
        for (auto it : obj)
        {
            filtered.push_back(it);
        }

        Verify::IsTrue(filtered.empty(), L"cannot contain any items after clear");
        Verify::VectorStrict(expected, initial);
    }

    void VerifyIsEmpty(initializer_list<int> initialE, bool expected)
    {
        vector<int> initial(initialE.begin(), initialE.end());

        Functor f;
        FilteredVectorType obj(initial, f);

        Verify::AreEqualLogOnError(expected, obj.IsEmpty, L"IsEmpty");
    }
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestFilteredVectorSuite, TestFilteredVector)

BOOST_AUTO_TEST_CASE(EmptyRange)
{
    RunAndVerify({ }, { });
}

BOOST_AUTO_TEST_CASE(SingleElementTrue)
{
    RunAndVerify({0}, {0});
}

BOOST_AUTO_TEST_CASE(SingleElementFalse)
{
    RunAndVerify({ 1 }, {});
}

BOOST_AUTO_TEST_CASE(LeadingElementTrue)
{
    RunAndVerify({ 0, 2, 4}, { 0, 2, 4});
}

BOOST_AUTO_TEST_CASE(LeadingElementFalse)
{
    RunAndVerify({ 1, 2, 4}, { 2, 4});
}

BOOST_AUTO_TEST_CASE(IntermediateElement)
{
    RunAndVerify({ 1, 2, 3, 4, 5, 7}, { 2, 4 });
}

BOOST_AUTO_TEST_CASE(ConstVector)
{
    vector<int> initial;
    initial.push_back(0); initial.push_back(1);

    vector<int> expected;
    expected.push_back(0);

    Functor f;
    const FilteredVectorType obj(initial, f);

    vector<int> actual;
    for (auto it = obj.begin(); it != obj.end(); ++it)
    {
        actual.push_back(*it);
    }

    Verify::Vector(expected, actual);
}

BOOST_AUTO_TEST_CASE(ClearEmpty)
{
    ClearAndVerify({}, {});
}

BOOST_AUTO_TEST_CASE(ClearWithNothingMatchingPredicate)
{
    ClearAndVerify({ 1, 3}, { 1, 3});
}

BOOST_AUTO_TEST_CASE(ClearWithEverythingMatchingPredicate)
{
    ClearAndVerify({ 0, 2}, { });
}

BOOST_AUTO_TEST_CASE(ClearWithPartialMatch)
{
    ClearAndVerify({ 1, 2, 3}, {1, 3});
}

BOOST_AUTO_TEST_CASE(IsEmptyTest)
{
    VerifyIsEmpty({}, true);
}

BOOST_AUTO_TEST_CASE(IsEmptyWithOnlyNonMatchingElements)
{
    VerifyIsEmpty({1, 3}, true);
}

BOOST_AUTO_TEST_CASE(IsEmptyWithMixOfElements)
{
    VerifyIsEmpty({ 1, 2, 0 }, false);
}

BOOST_AUTO_TEST_CASE(IsEmptyWithMatchingElemtents)
{
    VerifyIsEmpty({ 0 }, false);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
