//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include <chrono>
#include <thread>

using namespace std;
using namespace Common;
using namespace Hosting2;

const StringLiteral TraceType("OverlayIPAMTest");

static std::unordered_set<OverlayNetworkResourceSPtr> testDataToBeAdded;
static std::unordered_set<OverlayNetworkResourceSPtr> testDataToBeRemoved;
static std::unordered_set<OverlayNetworkResourceSPtr> additionalTestDataToBeAdded;
static std::unordered_set<OverlayNetworkResourceSPtr> additionalTestDataToBeRemoved;

class OverlayIPAMTest
{
protected:
    OverlayIPAMTest() { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup)
        ~OverlayIPAMTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup)

    void VerifyCounts(OverlayIPAM *overlayIpam, int total, int inuse, int avail);
};

BOOST_FIXTURE_TEST_SUITE(OverlayIPAMTestSuite, OverlayIPAMTest)

BOOST_AUTO_TEST_CASE(InitializationSuccessTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeAdded;
    dataToBeAdded.insert(testDataToBeAdded.begin(), testDataToBeAdded.end());

    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeRemoved;
    dataToBeRemoved.insert(testDataToBeRemoved.begin(), testDataToBeRemoved.end());

    auto ipam = new OverlayIPAM(
        root,
        // replenish callback
        []()
    {
        // There should never be a replenish call
        VERIFY_IS_TRUE(false);
    },
        // ghost callback
        [](DateTime lastRefresh)
    {
        UNREFERENCED_PARAMETER(lastRefresh);

        // There should never be a ghost
        VERIFY_IS_TRUE(false);
    },
        TimeSpan::FromMilliseconds(100));

    auto openErrorCode = ipam->Open();
    VERIFY_IS_TRUE(openErrorCode.IsSuccess());

    VERIFY_IS_FALSE(ipam->Initialized);

    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    
    VERIFY_IS_TRUE(ipam->Initialized);

    VerifyCounts(ipam, 3, 0, 3);

    auto closeErrorCode = ipam->Close();
    VERIFY_IS_TRUE(closeErrorCode.IsSuccess());
}

BOOST_AUTO_TEST_CASE(RefreshTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeAdded;
    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeRemoved;
    ManualResetEvent waiter;

    auto ipam = new OverlayIPAM(
        root,
        // replenish callback
        [&waiter]()
    {
        // should be called
        VERIFY_IS_TRUE(true);
        waiter.Set();
    },
        // ghost callback
        [](DateTime lastRefresh)
    {
        UNREFERENCED_PARAMETER(lastRefresh);

        // There should never be a ghost
        VERIFY_IS_TRUE(false);
    },
        TimeSpan::FromMilliseconds(2000));

    auto openErrorCode = ipam->Open();
    VERIFY_IS_TRUE(openErrorCode.IsSuccess());

    VERIFY_IS_FALSE(ipam->Initialized);

    // no data being added to so refresh should be invoked
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);

    VERIFY_IS_TRUE(ipam->Initialized);

    // Allow refresh cycle to happen so that callbacks can be validated
    waiter.Wait(5000);

    // add refresh data
    dataToBeAdded.insert(testDataToBeAdded.begin(), testDataToBeAdded.end());

    // refresh pool
    auto lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);

    VerifyCounts(ipam, 4, 0, 4);

    auto closeErrorCode = ipam->Close();
    VERIFY_IS_TRUE(closeErrorCode.IsSuccess());
}

BOOST_AUTO_TEST_CASE(GhostTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeAdded;
    dataToBeAdded.insert(testDataToBeAdded.begin(), testDataToBeAdded.end());

    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeRemoved;

    const wchar_t *names[] = { L"Reservation1", L"Reservation2", L"Reservation3", L"Reservation4" };
    OverlayNetworkResourceSPtr networkResources[4];
    ManualResetEvent replenishWaiter;
    bool ghostHit = false;

    auto ipam = new OverlayIPAM(
        root,
        // replenish callback
        [&replenishWaiter]()
    {
        // replenish will be called
        VERIFY_IS_TRUE(true);
        replenishWaiter.Set();
    },
        // ghost callback
        [&ghostHit](DateTime lastRefresh)
    {
        UNREFERENCED_PARAMETER(lastRefresh);
        ghostHit = true;
    },
        TimeSpan::FromMilliseconds(100));

    auto openErrorCode = ipam->Open();
    VERIFY_IS_TRUE(openErrorCode.IsSuccess());

    // data being added, no data being removed
    VERIFY_IS_FALSE(ipam->Initialized);
    auto lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    // refresh pool
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    VERIFY_IS_TRUE(ipam->Initialized);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);

    VerifyCounts(ipam, 4, 0, 4);

    // Use them all up
    //
    for (int i = 0; i < 4; i++)
    {
        auto reserveWaiter = make_shared<AsyncOperationWaiter>();
        OverlayNetworkResourceSPtr networkResource;

        ipam->BeginReserveWithRetry(
            names[i],
            TimeSpan::FromSeconds(1),
            [this, reserveWaiter, ipam, &networkResource](AsyncOperationSPtr const & operation)
        {
            auto err = ipam->EndReserveWithRetry(operation, networkResource);
            reserveWaiter->SetError(err);
            reserveWaiter->Set();
        },
            AsyncOperationSPtr());

        reserveWaiter->WaitOne();
        auto error = reserveWaiter->GetError();
        networkResources[i] = networkResource;
        VERIFY_IS_TRUE(error.IsSuccess());
    }

    VerifyCounts(ipam, 4, 4, 0);

    // Allow refresh cycle to happen so that replenish callback can be validated
    replenishWaiter.Wait(2000);

    // data removed
    dataToBeRemoved.insert(testDataToBeRemoved.begin(), testDataToBeRemoved.end());
    lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    //refresh pool
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    VERIFY_IS_TRUE(ipam->Initialized);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);

    VERIFY_IS_TRUE(ghostHit);
    VERIFY_ARE_EQUAL(ipam->GhostCount, 1);
    auto ghost = ipam->GetGhostReservations();
    VERIFY_ARE_EQUAL((int)ghost.size(), 1);
    auto ghostName = ghost.front();
    
    int ghostIndex = -1;
    for (int i = 0; i < 4; i++)
    {
        if (ghostName == names[i])
        {
            ghostIndex = i;
        }
    }

    VERIFY_ARE_NOT_EQUAL(ghostIndex, -1);
    VerifyCounts(ipam, 3, 3, 0);

    //refresh pool so that release will go through
    lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    ipam->OnNewIpamData(additionalTestDataToBeAdded, std::unordered_set<OverlayNetworkResourceSPtr>());
    VERIFY_IS_TRUE(ipam->Initialized);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);

    // Release the reservation and see that the ghost disappears.
    //
    auto releaseWaiter = make_shared<AsyncOperationWaiter>();

    ipam->BeginReleaseWithRetry(
        names[ghostIndex],
        TimeSpan::FromSeconds(1),
        [this, releaseWaiter, ipam](AsyncOperationSPtr const & operation) 
        { 
            auto err = ipam->EndReleaseWithRetry(operation);
            releaseWaiter->SetError(err); 
            releaseWaiter->Set();
        },
        AsyncOperationSPtr());

    releaseWaiter->WaitOne();
    auto error = releaseWaiter->GetError();

    VERIFY_IS_TRUE(error.IsSuccess());

    VERIFY_ARE_EQUAL(ipam->GhostCount, 0);
    ghost = ipam->GetGhostReservations();
    VERIFY_ARE_EQUAL((int)ghost.size(), 0);
    VerifyCounts(ipam, 4, 3, 1);

    // And finally, release all the reservations and make sure that they
    // work.
    //
    for (int i = 0; i < 4; i++)
    {
        if (i != ghostIndex)
        {
            auto releaseWaiter1 = make_shared<AsyncOperationWaiter>();

            ipam->BeginReleaseWithRetry(
                names[i],
                TimeSpan::FromSeconds(1),
                [this, releaseWaiter1, ipam](AsyncOperationSPtr const & operation)
            {
                auto err = ipam->EndReleaseWithRetry(operation);
                releaseWaiter1->SetError(err);
                releaseWaiter1->Set();
            },
                AsyncOperationSPtr());

            releaseWaiter1->WaitOne();
            error = releaseWaiter1->GetError();
            VERIFY_IS_TRUE(error.IsSuccess());
        }
    }

    VerifyCounts(ipam, 4, 0, 4);

    auto closeErrorCode = ipam->Close();
    VERIFY_IS_TRUE(closeErrorCode.IsSuccess());
}

BOOST_AUTO_TEST_CASE(ReserveReleaseTest)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeAdded;
    dataToBeAdded.insert(testDataToBeAdded.begin(), testDataToBeAdded.end());

    std::unordered_set<OverlayNetworkResourceSPtr> dataToBeRemoved;

    const wchar_t *names[] = { L"Reservation1", L"Reservation2", L"Reservation3", L"Reservation4" };
    OverlayNetworkResourceSPtr networkResources[4];
    ManualResetEvent replenishWaiter;
    bool ghostHit = false;

    auto ipam = new OverlayIPAM(
        root,
        // replenish callback
        []()
    {
        // replenish will be called
        VERIFY_IS_TRUE(true);
    },
        // ghost callback
        [&ghostHit](DateTime lastRefresh)
    {
        UNREFERENCED_PARAMETER(lastRefresh);
        // ghost will be called
        ghostHit = true;
    },
        TimeSpan::FromMilliseconds(100));

    auto openErrorCode = ipam->Open();
    VERIFY_IS_TRUE(openErrorCode.IsSuccess());

    // data being added, no data being removed
    VERIFY_IS_FALSE(ipam->Initialized);
    auto lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    // refresh pool
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    VERIFY_IS_TRUE(ipam->Initialized);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);

    VerifyCounts(ipam, 4, 0, 4);

    // Use them all up
    //
    for (int i = 0; i < 4; i++)
    {
        auto reserveWaiter = make_shared<AsyncOperationWaiter>();
        OverlayNetworkResourceSPtr networkResource;

        ipam->BeginReserveWithRetry(
            names[i],
            TimeSpan::FromSeconds(1),
            [this, reserveWaiter, ipam, &networkResource](AsyncOperationSPtr const & operation)
        {
            auto err = ipam->EndReserveWithRetry(operation, networkResource);
            reserveWaiter->SetError(err);
            reserveWaiter->Set();
        },
            AsyncOperationSPtr());

        reserveWaiter->WaitOne();
        auto error = reserveWaiter->GetError();
        networkResources[i] = networkResource;
        VERIFY_IS_TRUE(error.IsSuccess());
    }

    VerifyCounts(ipam, 4, 4, 0);

    // Try reserving/releasing one more time
    // Replenish callback should be called and 
    // since we do not add any more ips in the test refresh cycle, 
    // the status should be 'OperationFailed'
    OverlayNetworkResourceSPtr networkResource;
    auto reserveWaiter = make_shared<AsyncOperationWaiter>();

    ipam->BeginReserveWithRetry(
        L"Reservation5",
        TimeSpan::FromSeconds(1),
        [this, reserveWaiter, ipam, &networkResource](AsyncOperationSPtr const & operation)
    {
        auto err = ipam->EndReserveWithRetry(operation, networkResource);
        reserveWaiter->SetError(err);
        reserveWaiter->Set();
    },
        AsyncOperationSPtr());

    reserveWaiter->WaitOne();
    auto error = reserveWaiter->GetError();

    VERIFY_ARE_EQUAL(error.ErrorCodeValueToString(), ErrorCode(ErrorCodeValue::OperationFailed).ErrorCodeValueToString());

    auto releaseWaiter = make_shared<AsyncOperationWaiter>();
    ipam->BeginReleaseWithRetry(
        L"Reservation5",
        TimeSpan::FromSeconds(1),
        [this, releaseWaiter, ipam](AsyncOperationSPtr const & operation)
    {
        auto err = ipam->EndReleaseWithRetry(operation);
        releaseWaiter->SetError(err);
        releaseWaiter->Set();
    },
        AsyncOperationSPtr());

    releaseWaiter->WaitOne();
    error = releaseWaiter->GetError();
    VERIFY_ARE_EQUAL(error.ErrorCodeValueToString(), ErrorCode(ErrorCodeValue::OperationFailed).ErrorCodeValueToString());
   
    // Test ghost list is maintained
    dataToBeRemoved.insert(testDataToBeRemoved.begin(), testDataToBeRemoved.end());
    lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    //refresh pool
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    VERIFY_IS_TRUE(ipam->Initialized);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);
    VERIFY_IS_TRUE(ghostHit);

    // reset flag
    ghostHit = false;
    lastLoad = ipam->LastRefreshTime;
    this_thread::sleep_for(chrono::milliseconds(1));
    dataToBeRemoved.clear();
    dataToBeRemoved.insert(additionalTestDataToBeRemoved.begin(), additionalTestDataToBeRemoved.end());

    // refresh pool
    ipam->OnNewIpamData(dataToBeAdded, dataToBeRemoved);
    VERIFY_IS_TRUE(ipam->Initialized);
    VERIFY_ARE_NOT_EQUAL(lastLoad, ipam->LastRefreshTime);
    VERIFY_IS_TRUE(ghostHit);

    VERIFY_ARE_EQUAL(ipam->GhostCount, 2);
    auto ghost = ipam->GetGhostReservations();
    VERIFY_ARE_EQUAL((int)ghost.size(), 2);

    VerifyCounts(ipam, 3, 2, 1);

    auto closeErrorCode = ipam->Close();
    VERIFY_IS_TRUE(closeErrorCode.IsSuccess());
}

BOOST_AUTO_TEST_SUITE_END()

bool OverlayIPAMTest::Setup()
{
    testDataToBeAdded.insert(make_shared<OverlayNetworkResource>(1, 2));
    testDataToBeAdded.insert(make_shared<OverlayNetworkResource>(3, 4));
    testDataToBeAdded.insert(make_shared<OverlayNetworkResource>(5, 6));
    testDataToBeAdded.insert(make_shared<OverlayNetworkResource>(7, 8));

    additionalTestDataToBeAdded.insert(make_shared<OverlayNetworkResource>(9, 10));

    testDataToBeRemoved.insert(make_shared<OverlayNetworkResource>(5, 6));

    additionalTestDataToBeRemoved.insert(make_shared<OverlayNetworkResource>(1, 2));

    return true;
}

bool OverlayIPAMTest::Cleanup()
{
    return true;
}

void OverlayIPAMTest::VerifyCounts(OverlayIPAM *ipam, int total, int inuse, int avail)
{
    VERIFY_ARE_EQUAL(total, ipam->TotalCount);
    VERIFY_ARE_EQUAL(inuse, ipam->ReservedCount);
    VERIFY_ARE_EQUAL(avail, ipam->FreeCount);
}