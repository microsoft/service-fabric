// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

namespace
{
    EntitySetName::Enum FailoverManagerIdRequired[] =
    {
        EntitySetName::FailoverManagerMessageRetry,
        EntitySetName::ReplicaUploadPending,
    };

    EntitySetName::Enum FailoverManagerIdNotRequired[] =
    {
        EntitySetName::Invalid,
        EntitySetName::ReconfigurationMessageRetry,
        EntitySetName::StateCleanup,
        EntitySetName::ReplicaDown,
        EntitySetName::ReplicaCloseMessageRetry,
        EntitySetName::ReplicaUpMessageRetry,
        EntitySetName::ReplicaOpenMessageRetry,
        EntitySetName::UpdateServiceDescriptionMessageRetry,
    };

    EntitySetName::Enum All[] =
    {
        EntitySetName::Invalid,
        EntitySetName::ReconfigurationMessageRetry,
        EntitySetName::StateCleanup,
        EntitySetName::ReplicaDown,
        EntitySetName::ReplicaCloseMessageRetry,
        EntitySetName::ReplicaUpMessageRetry,
        EntitySetName::ReplicaOpenMessageRetry,
        EntitySetName::UpdateServiceDescriptionMessageRetry,
        EntitySetName::FailoverManagerMessageRetry,
        EntitySetName::ReplicaUploadPending,
    };
}

class TestEntitySetIdentifier
{
protected:
    void VerifyFmIdIsNotConsideredForEquality(EntitySetName::Enum name)
    {
        VerifyEqual(EntitySetIdentifier(name), EntitySetIdentifier(name));
        VerifyEqual(EntitySetIdentifier(name), EntitySetIdentifier(name, *FailoverManagerId::Fmm));
        VerifyEqual(EntitySetIdentifier(name), EntitySetIdentifier(name, *FailoverManagerId::Fm));
        VerifyEqual(EntitySetIdentifier(name, *FailoverManagerId::Fm), EntitySetIdentifier(name, *FailoverManagerId::Fm));
        VerifyEqual(EntitySetIdentifier(name, *FailoverManagerId::Fm), EntitySetIdentifier(name, *FailoverManagerId::Fmm));
        VerifyEqual(EntitySetIdentifier(name, *FailoverManagerId::Fmm), EntitySetIdentifier(name, *FailoverManagerId::Fmm));
    }

    void VerifyEqual(EntitySetIdentifier const & lhs, EntitySetIdentifier const & rhs)
    {
        Verify(lhs, rhs, true);
    }

    void Verify(EntitySetIdentifier const & lhs, EntitySetIdentifier const & rhs, bool expected)
    {        
        Verify::AreEqual(expected, lhs == rhs, L"Equality op");
        Verify::AreEqual(!expected, lhs != rhs, L"Inequality op");

        Verify::AreEqual(expected, rhs == lhs, L"Equality op rhs==lhs");
        Verify::AreEqual(!expected, rhs != lhs, L"Inequality op rhs != lhs");
    }
};


BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestEntitySetIdentifierSuite, TestEntitySetIdentifier)

BOOST_AUTO_TEST_CASE(FmIdNotConsideredForEquality_EqualityTest)
{
    for (auto it : FailoverManagerIdNotRequired)
    {
        TestLog::WriteInfo(wformatString(it));
        VerifyFmIdIsNotConsideredForEquality(it);
    }
}

BOOST_AUTO_TEST_CASE(FmIdNotConsideredForEquality_InEqualityTest)
{
    for (auto i : FailoverManagerIdNotRequired)
    {
        for (auto j : All)
        {
            if (i == j)
            {
                continue;
            }

            TestLog::WriteInfo(wformatString("{0} {1}", i, j));
            Verify(EntitySetIdentifier(i), EntitySetIdentifier(j), false);
        }
    }
}

BOOST_AUTO_TEST_CASE(FMIdConsidered_EqualityTest)
{
    for (auto it : FailoverManagerIdRequired)
    {
        TestLog::WriteInfo(wformatString(it));

        auto fm = EntitySetIdentifier(it, *FailoverManagerId::Fmm);
        auto fmm = EntitySetIdentifier(it, *FailoverManagerId::Fm);
        Verify(fm, fm, true);
        Verify(fm, fmm, false);
        Verify(fmm, fm, false);
        Verify(fmm, fmm, true);
    }
}

BOOST_AUTO_TEST_CASE(FMMessageRetryWithOther)
{
    for (auto i : FailoverManagerIdRequired)
    {
        auto fm = EntitySetIdentifier(i, *FailoverManagerId::Fmm);
        auto fmm = EntitySetIdentifier(i, *FailoverManagerId::Fm);

        for (auto j : All)
        {
            if (i == j)
            {
                continue;
            }

            TestLog::WriteInfo(wformatString("{0} {1}", i, j));
            Verify(fm, EntitySetIdentifier(j), false);
            Verify(fm, EntitySetIdentifier(j, *FailoverManagerId::Fm), false);
            Verify(fm, EntitySetIdentifier(j, *FailoverManagerId::Fmm), false);

            Verify(fmm, EntitySetIdentifier(j), false);
            Verify(fmm, EntitySetIdentifier(j, *FailoverManagerId::Fm), false);
            Verify(fmm, EntitySetIdentifier(j, *FailoverManagerId::Fmm), false);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
