// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

using namespace ReliabilityUnitTest;
using namespace Infrastructure;

namespace StateValues
{
    enum Enum
    {
        // The state should be null
        Null,

        // The state should be deleted
        Deleted,

        // The state should have updated value (UpgradePending = true)
        Updated,

        // The state should have initial value (UpgradePending = false)
        Default
    };
}

class TestLockedFailoverUnitPtr
{
protected:
    TestLockedFailoverUnitPtr() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~TestLockedFailoverUnitPtr() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void TestInitialize(StateValues::Enum value);
    void UpdateState(FailoverUnit & ft);
    void VerifyState(StateValues::Enum value);
    void VerifyFlagValue(StateValues::Enum expected, FailoverUnit const & ft);

    FailoverUnitSPtr CreateDefaultFT() const
    {
        return FailoverUnitContainer::CreateClosedFTPtr(GetContext());
    }

    StateManagement::SingleFTReadContext const & GetContext() const
    {
        return StateManagement::Default::GetInstance().SP1_FTContext;
    }

    unique_ptr<UnitTestContext> testContext_;
    LocalFailoverUnitMapEntrySPtr entry_;
    FailoverUnitContainerUPtr ftContainer_;
};

bool TestLockedFailoverUnitPtr::TestSetup()
{
    testContext_ = UnitTestContext::Create(UnitTestContext::Option::TestAssertEnabled);
	return true;
}

bool TestLockedFailoverUnitPtr::TestCleanup()
{
	testContext_->Cleanup();
	return true;
}

void TestLockedFailoverUnitPtr::TestInitialize(StateValues::Enum value)
{
    auto const & context = GetContext();

    if (value == StateValues::Deleted)
    {
        entry_ = make_shared<LocalFailoverUnitMapEntry>(context.FUID);
        entry_->State.Test_Data.Delete();
    }
    else if (value == StateValues::Default)
    {
        auto ft = CreateDefaultFT();
        auto id = ft->FailoverUnitId;
        entry_ = make_shared<LocalFailoverUnitMapEntry>(id, move(ft));
    }
    else if (value == StateValues::Null)
    {
        entry_ = make_shared<LocalFailoverUnitMapEntry>(context.FUID);
    }
    else
    {
        Verify::Fail(wformatString("Unexpected value {0}", static_cast<int>(value)));
    }
}

void TestLockedFailoverUnitPtr::UpdateState(FailoverUnit & ft)
{
    ft.UpgradePending = true;
}

void TestLockedFailoverUnitPtr::VerifyFlagValue(StateValues::Enum expected, FailoverUnit const &ft)
{
    bool flagValue = expected == StateValues::Updated;
    Verify::AreEqualLogOnError(flagValue, ft.UpgradePending, L"Flag value");
}

void TestLockedFailoverUnitPtr::VerifyState(StateValues::Enum value)
{
    bool isDeletedValue = false;
    bool shouldBeNull = false;

    if (value == StateValues::Deleted)
    {
        isDeletedValue = true;
        shouldBeNull = true;
    }
    else if (value == StateValues::Default)
    {
        isDeletedValue = false;
        shouldBeNull = false;
    }
    else if (value == StateValues::Null)
    {
        isDeletedValue = false;
        shouldBeNull = true;
    }
    else if (value == StateValues::Updated)
    {
        isDeletedValue = false;
        shouldBeNull = false;
    }

    auto const & snapshot = entry_->State.Test_Data;
    Verify::AreEqualLogOnError(isDeletedValue, snapshot.IsDeleted, L"Expected entry to be deleted");
    Verify::AreEqualLogOnError(shouldBeNull, snapshot.Data == nullptr, L"Expected FT to be null");

    if (!shouldBeNull)
    {
        VerifyFlagValue(value, *snapshot.Data);
    }
}

BOOST_FIXTURE_TEST_SUITE(TestLockedFailoverUnitPtrSuite,TestLockedFailoverUnitPtr)


BOOST_AUTO_TEST_CASE(EnableUpdateAndCommitResetsFailoverUnitState)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFUPtr(entry_);

    // Enable update
    lockedFUPtr.EnableUpdate();
    UpdateState(*lockedFUPtr);
    lockedFUPtr.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Updated);
}

BOOST_AUTO_TEST_CASE(MultipleEnableUpdateIsAllowed)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFUPtr(entry_);

    // Enable update
    lockedFUPtr.EnableUpdate();
    lockedFUPtr.EnableUpdate();
    lockedFUPtr.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Default);
}

BOOST_AUTO_TEST_CASE(DestroyingUpdatedLockedFailoverUnitReverts)
{
    TestInitialize(StateValues::Default);

    {
        LockedFailoverUnitPtr lockedFUPtr(entry_);
        lockedFUPtr.EnableUpdate();
        UpdateState(*lockedFUPtr);
    }

    VerifyState(StateValues::Default);
}

BOOST_AUTO_TEST_CASE(InsertEnableUpdateAndCommitIsAllowed)
{
    TestInitialize(StateValues::Null);

    LockedFailoverUnitPtr lockedFT(entry_);
    lockedFT.Insert(CreateDefaultFT());
    lockedFT.EnableUpdate();
    UpdateState(*lockedFT);
    lockedFT.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Updated);
}

BOOST_AUTO_TEST_CASE(InsertAndCommitUpdatesFailoverUnitState)
{
    TestInitialize(StateValues::Null);
    
    LockedFailoverUnitPtr lockedFT(entry_);
    lockedFT.Insert(CreateDefaultFT());
    lockedFT.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Default);
}

BOOST_AUTO_TEST_CASE(EnableUpdateAndDeleteDeletesFT)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFT(entry_);
    lockedFT.EnableUpdate();
    lockedFT.MarkForDelete();
    lockedFT.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Deleted);
}

BOOST_AUTO_TEST_CASE(DeleteAndCommitDeletesFailoverUnitState)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFT(entry_);
    lockedFT.MarkForDelete();
    lockedFT.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Deleted);
}

BOOST_AUTO_TEST_CASE(EntryIsUnlockedAfterLockedFTIsDestroyed)
{
    TestInitialize(StateValues::Default);

    {
        LockedFailoverUnitPtr lockedFT(entry_);
    }

    // Another lock should be successful
    {
        LockedFailoverUnitPtr lockedFT(entry_);
    }
}

BOOST_AUTO_TEST_CASE(MultipleLocksShouldAssert)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFT(entry_);

    Verify::Asserts([this]() { LockedFailoverUnitPtr another(entry_); }, L"Second lock did not assert");
}

BOOST_AUTO_TEST_CASE(AcquiringWriteLockShouldAllowForWritesWithoutCorruptingState)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFT(entry_);

    lockedFT.EnableUpdate();
    UpdateState(*lockedFT);

    VerifyState(StateValues::Default);

    lockedFT.Test_Commit(testContext_->RA);

    VerifyState(StateValues::Updated);
}
BOOST_AUTO_TEST_CASE(EmptyLockedFailoverUnitPtrIsInvalid)
{
    LockedFailoverUnitPtr lockedFT;

    Verify::Asserts([&lockedFT, this]() { lockedFT.Test_Commit(testContext_->RA); }, L"Invalid FT cannot be asked to commit");
    Verify::Asserts([&lockedFT]() { lockedFT.Current; }, L"Invalid FT cannot be asked for current");
    Verify::Asserts([&lockedFT]() { lockedFT.EnableUpdate(); }, L"Invalid FT cannot be asked for enable update");
    Verify::Asserts([&lockedFT]() { lockedFT.MarkForDelete(); }, L"Invalid FT cannot be asked for delete");
    Verify::Asserts([&lockedFT]() { lockedFT.IsUpdating; }, L"Invalid FT cannot be asked for isupdating");
}

BOOST_AUTO_TEST_CASE(MoveAssignmentForLockedPtr)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFT;

    LockedFailoverUnitPtr lock(entry_);
    lock.EnableUpdate();

    lockedFT = move(lock);

    Verify::IsTrue(lockedFT.IsUpdating, L"Constructor should have transferred data");
    Verify::AreEqual(entry_, lockedFT.Entry, L"Entry");
}

BOOST_AUTO_TEST_CASE(MoveAssignmentToLockedPtrAsserts)
{
    TestInitialize(StateValues::Default);

    LockedFailoverUnitPtr lockedFT;

    LockedFailoverUnitPtr lock(entry_);

    Verify::Asserts([&lockedFT, &lock]() { lock = move(lockedFT); }, L"Move assignment must assert");
}

BOOST_AUTO_TEST_CASE(ReadLockHasTheCorrectValueOfIsDeleted)
{
    TestInitialize(StateValues::Deleted);

    ReadOnlyLockedFailoverUnitPtr lockedFT(entry_);

    Verify::IsTrue(lockedFT.IsEntityDeleted, L"Entity must be deleted");
}

BOOST_AUTO_TEST_CASE(ReadLockHasOldValueAfterWriteLockUpdates)
{
    TestInitialize(StateValues::Default);

    ReadOnlyLockedFailoverUnitPtr readLock(entry_);

    {
        LockedFailoverUnitPtr writeLock(entry_);
        writeLock.EnableUpdate();
        UpdateState(*writeLock);
        writeLock.Test_Commit(testContext_->RA);
    }

    VerifyFlagValue(StateValues::Default, *readLock);
}

BOOST_AUTO_TEST_CASE(ReadLockDoesNotChangeOnUpdatingWriteLock)
{
    TestInitialize(StateValues::Default);

    ReadOnlyLockedFailoverUnitPtr readLock(entry_);

    LockedFailoverUnitPtr writeLock(entry_);
    writeLock.EnableUpdate();
    UpdateState(*writeLock);

    VerifyFlagValue(StateValues::Default, *readLock);
}

BOOST_AUTO_TEST_CASE(MultipleReadLocksAreAllowed)
{
    TestInitialize(StateValues::Default);

    ReadOnlyLockedFailoverUnitPtr readLock1(entry_);
    ReadOnlyLockedFailoverUnitPtr readLock2(entry_);

    VerifyFlagValue(StateValues::Default, *readLock1);
    VerifyFlagValue(StateValues::Default, *readLock2);
}

BOOST_AUTO_TEST_CASE(EmptyReadOnlyLockedEntityPtrIsInvalid)
{
    ReadOnlyLockedFailoverUnitPtr readLock(entry_);
    readLock;
}

BOOST_AUTO_TEST_SUITE_END()
