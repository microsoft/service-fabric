// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Store/test/Common.Test.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace Reliability::ReplicationComponent;
using namespace std;

#define VERIFY( condition, fmt, ...) \
    { \
    wstring tmp; \
    StringWriter writer(tmp); \
    writer.Write(fmt, __VA_ARGS__); \
    VERIFY_IS_TRUE( condition, tmp.c_str() ); \
} \

namespace Store
{
    StringLiteral const TraceComponent("HealthTrackerTest");

    class ComMockPartition 
        : public IFabricStatefulServicePartition3
        , private Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(ComMockPartition)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricStatefulServicePartition3)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition,IFabricStatefulServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition1,IFabricStatefulServicePartition1)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition2,IFabricStatefulServicePartition2)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition3, IFabricStatefulServicePartition3)
        END_COM_INTERFACE_LIST()

    public:

        virtual ~ComMockPartition() {};

    public: // Test

        void ValidateReportCount(long expectedCount)
        {
            auto count = healthReportCount_.load();
            bool success = (count == expectedCount);

            if (!success)
            {
                auto msg = wformatString("expected={0} actual={1}", expectedCount, count);
                Trace.WriteError(TraceComponent, "{0}", msg);
                VERIFY(success, "{0}", msg);
            }
        }

    public: 
        
        // IFabricStatefulServicePartition3

        virtual HRESULT ReportReplicaHealth2(const FABRIC_HEALTH_INFORMATION * publicInfo, const FABRIC_HEALTH_REPORT_SEND_OPTIONS *) override
        { 
            HealthInformation internalInfo;
            auto error = internalInfo.FromCommonPublicApi(*publicInfo);
            VERIFY(error.IsSuccess(), "FromCommonPublicApi: error={0}", error);

            auto count = ++healthReportCount_;
            Trace.WriteInfo(TraceComponent, "ComMockPartition::ReportReplicaHealth({0}) total={1}", internalInfo, count);
            return S_OK;
        };

        HRESULT ReportPartitionHealth2(const FABRIC_HEALTH_INFORMATION *, const FABRIC_HEALTH_REPORT_SEND_OPTIONS *) override { return E_NOTIMPL; }

        // IFabricStatefulServicePartition2

        virtual HRESULT ReportReplicaHealth(const FABRIC_HEALTH_INFORMATION * publicInfo) override 
        {
            return ReportReplicaHealth2(publicInfo, nullptr);
        };

        HRESULT ReportPartitionHealth(const FABRIC_HEALTH_INFORMATION *) override { return E_NOTIMPL; }

        // IFabricStatefulServicePartition1

        HRESULT ReportMoveCost(FABRIC_MOVE_COST) override { return E_NOTIMPL; }

        // IFabricStatefulServicePartition
        
        HRESULT GetPartitionInfo(const FABRIC_SERVICE_PARTITION_INFORMATION **) override { return E_NOTIMPL; }

        HRESULT GetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS *) override { return E_NOTIMPL; }

        HRESULT GetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS *) override { return E_NOTIMPL; }

        HRESULT CreateReplicator(
            IFabricStateProvider *,
            FABRIC_REPLICATOR_SETTINGS const *,
            IFabricReplicator **,
            IFabricStateReplicator **) override { return E_NOTIMPL; }

        HRESULT ReportLoad(
            ULONG,
            const FABRIC_LOAD_METRIC *) override { return E_NOTIMPL; }

        HRESULT ReportFault(FABRIC_FAULT_TYPE) override { return E_NOTIMPL; }

    private:
        Common::atomic_long healthReportCount_;

    }; // ComMockPartition

    class HealthTrackerTest
    {
    public:
        HealthTrackerTest() { BOOST_REQUIRE(MethodSetup()); }
        TEST_METHOD_SETUP(MethodSetup);
        ~HealthTrackerTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP(MethodCleanup);
    };

#if !defined(PLATFORM_UNIX)
    void WaitWithBuffer(TimeSpan const delay)
    {
        Sleep(static_cast<DWORD>(delay.TotalPositiveMilliseconds() + 50));
    }

    BOOST_AUTO_TEST_CASE(SlowCommitTest)
    {
        StoreConfig::GetConfig().SlowCommitCountThreshold = 10;
        StoreConfig::GetConfig().SlowCommitTimeThreshold = TimeSpan::FromSeconds(10);

        auto thresholdCount = StoreConfig::GetConfig().SlowCommitCountThreshold;
        auto thresholdTime = StoreConfig::GetConfig().SlowCommitTimeThreshold;

        FABRIC_REPLICATOR_SETTINGS replicatorSettings = {0};
        ReplicatorSettingsUPtr replicatorSettingsUPtr;
        auto error = ReplicatorSettings::FromPublicApi(replicatorSettings, replicatorSettingsUPtr);
        VERIFY(error.IsSuccess(), "ReplicatorSettings::FromPublicApi error={0}", error);

        ComponentRoot root;
        auto store = EseReplicatedStore::Create(
            Guid::NewGuid(),
            DateTime::Now().Ticks, 
            move(replicatorSettingsUPtr),
            ReplicatedStoreSettings(),
            EseLocalStoreSettings(
                L"HealthTrackerTest.edb", 
                L".\\"), 
            Api::IStoreEventHandlerPtr(),
            Api::ISecondaryEventHandlerPtr(),
            root);

        auto castedStore = reinterpret_cast<ReplicatedStore*>(store.get());

        auto mockPartition = make_com<ComMockPartition>();

        ComPointer<IFabricStatefulServicePartition> comPartition;
        auto hr = mockPartition->QueryInterface(IID_IFabricStatefulServicePartition, comPartition.VoidInitializationAddress());
        VERIFY(SUCCEEDED(hr), "QueryInterface(IID_IFabricStatefulServicePartition): hr={0}", hr);

        shared_ptr<ReplicatedStore::HealthTracker> healthTracker;
        error = ReplicatedStore::HealthTracker::Create(*castedStore, comPartition, healthTracker);
        VERIFY(error.IsSuccess(), "HealthTracker::Create(): error={0}", error);

        auto refreshTime = healthTracker->ComputeRefreshTime(thresholdTime);

        // Testcase: under count threshold
        //
        Trace.WriteInfo(TraceComponent, "*** Expect no reports"); 

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);

        for (auto ix=0; ix<thresholdCount-1; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(0);
        
        // Testcase: meet time threshold
        //
        Trace.WriteInfo(TraceComponent, "*** Expect no reports"); 

        WaitWithBuffer(thresholdTime);

        healthTracker->OnSlowCommit(); // Put index at last element to test circular buffer

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(0);
        
        // Testcase: meet count threshold
        //
        Trace.WriteInfo(TraceComponent, "*** Expect report"); 

        WaitWithBuffer(thresholdTime);

        for (auto ix=0; ix<thresholdCount; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(1);
        
        // Testcase: meet refresh threshold (burst)
        //
        Trace.WriteInfo(TraceComponent, "*** Expect refresh (burst)"); 

        WaitWithBuffer(refreshTime);

        for (auto ix=0; ix<thresholdCount / 2 - 1; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(2);

        // Testcase: continue to meet refresh threshold (trickle)
        //
        Trace.WriteInfo(TraceComponent, "*** Expect refresh (trickle)"); 

        for (auto ix=0; ix<thresholdCount / 2 + 1; ++ix)
        {
            healthTracker->OnSlowCommit();
            Sleep(1000);
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(3);

        // Testcase: increase config threshold
        //
        WaitWithBuffer(refreshTime);

        Trace.WriteInfo(TraceComponent, "*** Increase config threshold"); 

        StoreConfig::GetConfig().SlowCommitCountThreshold = thresholdCount * 2;

        for (auto ix=0; ix<thresholdCount; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(3);

        for (auto ix=0; ix<thresholdCount; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(4);
        
        // Testcase: decrease config threshold
        //
        WaitWithBuffer(refreshTime);

        Trace.WriteInfo(TraceComponent, "*** Decrease config threshold"); 

        StoreConfig::GetConfig().SlowCommitCountThreshold = thresholdCount / 2;

        for (auto ix=0; ix<thresholdCount / 2; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(5);

        // Testcase: suppress multiple reports within refresh time
        //
        WaitWithBuffer(refreshTime);

        Trace.WriteInfo(TraceComponent, "*** Suppress multiple reports"); 

        for (auto ix=0; ix<thresholdCount * 2; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(6);

        // Testcase: disable reports
        //
        WaitWithBuffer(refreshTime);

        Trace.WriteInfo(TraceComponent, "*** Disable reports"); 
        
        StoreConfig::GetConfig().SlowCommitCountThreshold = 0;

        for (auto ix=0; ix<thresholdCount; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(6);

        // Testcase: re-enable reports
        //
        WaitWithBuffer(refreshTime);

        Trace.WriteInfo(TraceComponent, "*** Re-enable reports"); 

        StoreConfig::GetConfig().SlowCommitCountThreshold = thresholdCount;

        for (auto ix=0; ix<thresholdCount; ++ix)
        {
            healthTracker->OnSlowCommit();
        }

        Trace.WriteInfo(TraceComponent, "tracker={0}", *healthTracker);
        mockPartition->ValidateReportCount(7);
    }
#endif

    bool HealthTrackerTest::MethodSetup()
    {
        StoreConfig::GetConfig().Test_Reset();
        return true;
    }

    bool HealthTrackerTest::MethodCleanup()
    {
        return true;
    }
}
