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

class ReadWriteStatusStateTest
{
protected:
    bool TryUpdate(AccessStatus::Enum read, AccessStatus::Enum write)
    {
        return state_.TryUpdate(read, write);
    }

    void UpdateAndVerify(AccessStatus::Enum read, AccessStatus::Enum write, bool expected)
    {
        auto actual = TryUpdate(read, write);
        Verify::AreEqual(expected, actual, L"TryUpdate return value");
    }

    void Verify(AccessStatus::Enum readStatus, AccessStatus::Enum writeStatus)
    {
        VerifyReadStatus(readStatus);
        VerifyWriteStatus(writeStatus);
    }

    void VerifyWriteStatus(AccessStatus::Enum expected)
    {
        auto actual = Get(AccessStatusType::Write);
        Verify::AreEqual(expected, actual, L"WriteStatus");
    }

    void VerifyReadStatus(AccessStatus::Enum expected)
    {
        auto actual = Get(AccessStatusType::Read);
        Verify::AreEqual(expected, actual, L"ReadStatus");
    }

    AccessStatus::Enum Get(AccessStatusType::Enum type)
    {
        return Get(state_.Current, type);
    }

    static AccessStatus::Enum Get(ReadWriteStatusValue const &value, AccessStatusType::Enum type)
    {
        AccessStatus::Enum rv;        
        auto error = value.TryGet(type, rv);
        Verify::IsTrueLogOnError(error.IsSuccess(), L"Get cannot fail");
        return rv;
    }

    ReadWriteStatusState state_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(ReadWriteStatusStateTestSuite, ReadWriteStatusStateTest)

BOOST_AUTO_TEST_CASE(InitialReadWriteStatusIsConsistent)
{
    // This test verifies that the initial read/write status of a partition
    // Is the same as the initial read write status of the FUP
    ReadWriteStatusValue expected;
    VerifyReadStatus(Get(expected, AccessStatusType::Read));
    VerifyWriteStatus(Get(expected, AccessStatusType::Read));
}

BOOST_AUTO_TEST_CASE(TryUpdateWithDifferentReadStatusReturnsTrue)
{
    UpdateAndVerify(AccessStatus::Granted, AccessStatus::NotPrimary, true);
}

BOOST_AUTO_TEST_CASE(TryUpdateWithDifferentWriteStatusReturnsTrue)
{
    UpdateAndVerify(AccessStatus::NotPrimary, AccessStatus::Granted, true);
}

BOOST_AUTO_TEST_CASE(MultipleReadStatusUpdate)
{
    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    UpdateAndVerify(AccessStatus::NotPrimary, AccessStatus::Granted, true);
}

BOOST_AUTO_TEST_CASE(MultipleWriteStatusUpdate)
{
    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    UpdateAndVerify(AccessStatus::Granted, AccessStatus::NotPrimary, true);
}

BOOST_AUTO_TEST_CASE(BothChangeAtTheSameTime)
{
    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    UpdateAndVerify(AccessStatus::NotPrimary, AccessStatus::NotPrimary, true);
}

BOOST_AUTO_TEST_CASE(CurrentAfterSuccessfulWriteUpdate)
{
    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    TryUpdate(AccessStatus::Granted, AccessStatus::NotPrimary);

    Verify(AccessStatus::Granted, AccessStatus::NotPrimary);
}

BOOST_AUTO_TEST_CASE(CurrentAfterSuccessfulReadUpdate)
{
    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    TryUpdate(AccessStatus::NotPrimary, AccessStatus::Granted);

    Verify(AccessStatus::NotPrimary, AccessStatus::Granted);
}

BOOST_AUTO_TEST_CASE(CurrentAfterNoChange)
{
    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    TryUpdate(AccessStatus::Granted, AccessStatus::Granted);

    Verify(AccessStatus::Granted, AccessStatus::Granted);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
