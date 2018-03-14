// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TransactionalReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace TxnReplicator::TestCommon;
    using namespace Data::Utilities;
    using namespace Common;

    StringLiteral const TraceComponent = "IOMonitorTests";

    class IOMonitorTests
    {
    protected:
        IOMonitorTests()
        {
            healthClient_ = TestHealthClient::Create(true);
            CommonConfig config; // load config object for tracing functionality

            int seed = GetTickCount();
            Common::Random r(seed);
            rId_ = r.Next();
            pId_.CreateNew();

            ConfigureSettings();
        }

        ~IOMonitorTests()
        {
        }

        void EndTest();

        void ConfigureSettings(
            __in ULONG count = 5,
            __in ULONG timeThresholdInSeconds = 1,
            __in ULONG healthReportTTL = 1)
        {
            timeThresholdSeconds_ = timeThresholdInSeconds;

            txrSettings_ = { 0 };
            txrSettings_.SlowLogIOCountThreshold = count;
            txrSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD;
            txrSettings_.SlowLogIOTimeThresholdSeconds = timeThresholdInSeconds;
            txrSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS;
            txrSettings_.SlowLogIOHealthReportTTLSeconds = healthReportTTL;
            txrSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS;
        }

        void Initialize(
            __in KAllocator & allocator)
        {
            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator);

            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings_, tmp);

            config_ = TRInternalSettings::Create(
                move(tmp),
                make_shared<TransactionalReplicatorConfig>());

            ioTracker_ = IOMonitor::Create(
                *prId_,
                L"Test",
                Common::SystemHealthReportCode::TR_SlowIO,
                config_,
                healthClient_,
                allocator);
        };

        void SlowIO(int count)
        {
            for (int i = 0; i < count; i++)
            {
                ioTracker_->OnSlowOperation();
            }
        }

        void UpdateConfig(wstring name, wstring newvalue, ConfigSettings & settings, shared_ptr<ConfigSettingsConfigStore> & store)
        {
            ConfigSection section;
            section.Name = L"TransactionalReplicator2";

            ConfigParameter d1;
            d1.Name = name;
            d1.Value = newvalue;

            section.Parameters[d1.Name] = d1;
            settings.Sections[section.Name] = section;

            store->Update(settings);
        }

        Awaitable<void> RandomSlowIO(int count, int minDelayMs, int maxDelayMs)
        {
            DWORD tSeed = GetTickCount();
            Common::Random random(tSeed);

            KTimer::SPtr localTimer;
            NTSTATUS status = KTimer::Create(localTimer, underlyingSystem_->NonPagedAllocator(), KTL_TAG_TEST);
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to initialize timer");

            for (int i = 0; i < count; i++)
            {
                if(minDelayMs > 0 && maxDelayMs > 0)
                {
                    co_await localTimer->StartTimerAsync(random.Next(minDelayMs, maxDelayMs), nullptr);
                }

                ioTracker_->OnSlowOperation();
                localTimer->Reuse();
            }
        }

        TestHealthClientSPtr healthClient_;
        TxnReplicator::TRInternalSettingsSPtr config_;
        TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings_;

        IOMonitor::SPtr ioTracker_;
        ULONG timeThresholdSeconds_;

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;

    private:
        KtlSystem * CreateKtlSystem()
        {
            KtlSystem* underlyingSystem;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            underlyingSystem->SetStrictAllocationChecks(TRUE);

            return underlyingSystem;
        }
    };

    void IOMonitorTests::EndTest()
    {
        prId_.Reset();
        ioTracker_.Reset();
        config_.reset();
        healthClient_.reset();
    }

    BOOST_FIXTURE_TEST_SUITE(TRHealthTrackerTestSuite, IOMonitorTests)

    BOOST_AUTO_TEST_CASE(Verify_HealthReport)
    {
        TR_TEST_TRACE_BEGIN("Verify_HealthReport")
        {
            UNREFERENCED_PARAMETER(allocator);

            // Count = 5, Time Threshold = 1s
            Initialize(allocator);

            // Report 5 slow log io ops w/no delay
            // Ensures all reports are fired within 1 time threshold
            SlowIO(5);

            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 1, "Expected 1 health report");

            // Delay one time threshold + 1
            Sleep(1000 * (timeThresholdSeconds_ + 1));

            // Report count - 1 slow ops
            SlowIO(4);

            // Confirm another health report has not been fired
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 1, "Expected 1 health report");

            EndTest();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_NoHealthReport)
    {
        TR_TEST_TRACE_BEGIN("Verify_NoHealthReport")
        {
            UNREFERENCED_PARAMETER(allocator);

            // Count = 5, Time Threshold = 1s
            Initialize(allocator);

            // Report 3 slow log io ops w/no delay
            SlowIO(3);

            // Expect no health report
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 0, "Expected 0 health reports");

            // Delay one time threshold + 1
            Sleep(1000 * (timeThresholdSeconds_ + 1));

            // Report 3 more slow operations
            SlowIO(3);

            // Confirm no health reports have still been fired
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 0, "Expected 0 health reports");

            EndTest();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_Null_IReplicatorHealthClient_NoHealthReport)
    {
        TR_TEST_TRACE_BEGIN("Verify_Null_IReplicatorHealthClient_NoHealthReport")
        {
            UNREFERENCED_PARAMETER(allocator);

            // Count = 5, Time Threshold = 1s
            // Set IReplicatorHealthClientSPtr to nullptr
            healthClient_ = nullptr;
            Initialize(allocator);

            // Report 15 slow log io ops w/no delay
            SlowIO(15);

            // Assert healthClient_ == null after 15 reported 'slow' operations' with count threshold of 5
            // Calling 'ReportReplicatorHealth' on nullptr is expected to AV.
            // Reaching the assert statement and exiting without issue confirms the nullptr case is handled by IOMonitor
            ASSERT_IFNOT(healthClient_ == nullptr, "Expected 0 health reports");
            EndTest();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_DisabledViaConfig_NoHealthReport)
    {
        TR_TEST_TRACE_BEGIN("Verify_DisabledViaConfig_NoHealthReport")
        {
            UNREFERENCED_PARAMETER(allocator);

            // Count = 0, Time Threshold = 1s
            ConfigureSettings(0, 1, 15);
            Initialize(allocator);

            // Report 15 slow log io ops w/no delay
            SlowIO(15);

            // Expect no health report
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 0, "Expected 0 health reports");

            // Delay one time threshold + 1
            Sleep(1000 * (timeThresholdSeconds_ + 1));

            // Report 30 more slow operations
            SlowIO(30);

            // Confirm no health reports have still been fired
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 0, "Expected 0 health reports");

            EndTest();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_DynamicConfigUpdate_HealthReport)
    {
        TR_TEST_TRACE_BEGIN("Verify_DynamicConfigUpdate_HealthReport")
        {
            UNREFERENCED_PARAMETER(allocator);
            
            ConfigSettings settings;
            auto configStore = make_shared<ConfigSettingsConfigStore>(settings);
            Config::SetConfigStore(configStore);

            // Count = 0, Time Threshold = 1s
            // Count is intentionally not set as dynamic config update will not override user settings
            txrSettings_ = { 0 };
            txrSettings_.SlowLogIOTimeThresholdSeconds = 1;
            txrSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS;
            txrSettings_.SlowLogIOHealthReportTTLSeconds = 1;
            txrSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS;

            Initialize(allocator);

            // Report 15 slow log io ops w/no delay
            SlowIO(15);

            // Expect no health report
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 0, "Expected 0 health reports");

            // Dynamically update count threshold
            UpdateConfig(L"SlowLogIOCountThreshold", L"20", settings, configStore);

            // Delay one time threshold + 1
            Sleep(1000 * (timeThresholdSeconds_ + 1));

            // Report 30 more slow operations
            SlowIO(30);

            // Confirm 1 health report has been fired
            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 1, "Expected 1 health report");

            EndTest();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_HealthReport_Stress)
    {
        TR_TEST_TRACE_BEGIN("Verify_HealthReport_Stress")
        {
            UNREFERENCED_PARAMETER(allocator);

            // Count = 20, Time Threshold = 1s
            ConfigureSettings(20, 1);
            Initialize(allocator);

            // Report 200 slow log io ops w/no delay
            // Ensures all reports are fired within 1 time threshold
            SlowIO(200);

            ASSERT_IFNOT(healthClient_->NumberofReportsSent == 1, "Expected 1 health report");

            // Delay one time threshold + 1
            Sleep(1000 * (timeThresholdSeconds_ + 1));

            // Report count threshold # of slow ops
            SlowIO(21);

            // Confirm another health report has been fired
            ASSERT_IFNOT(
                healthClient_->NumberofReportsSent == 2,
                "Expected 1 health report. {0}",
                *ioTracker_);

            EndTest();
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
