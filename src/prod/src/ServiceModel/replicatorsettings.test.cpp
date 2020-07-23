// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ServiceModelTests
{
    using namespace std;
    using namespace Common;
    using namespace ServiceModel;
    using namespace Reliability::ReplicationComponent;

    StringLiteral const TestSource("ReplicatorSettingsTest");

    class ReplicatorSettingsTest
    {
    protected:
        bool AreEqual(FABRIC_REPLICATOR_SETTINGS *original, ReplicatorSettings *output);
    };


    BOOST_FIXTURE_TEST_SUITE(ReplicatorSettingsTestSuite,ReplicatorSettingsTest)

    BOOST_AUTO_TEST_CASE(ConversionTest)
    {
        {
            Trace.WriteInfo(TestSource, "BasicSettings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
            publicReplicatorSettings.BatchAcknowledgementIntervalMilliseconds = 1;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialCopyQueueSize = 2;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialReplicationQueueSize = 2;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxCopyQueueSize = 4;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxReplicationQueueSize = 4;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_ADDRESS;
            publicReplicatorSettings.ReplicatorAddress = L"address1";

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK;
            publicReplicatorSettings.RequireServiceAck = true;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_RETRY_INTERVAL;
            publicReplicatorSettings.RetryIntervalMilliseconds = 7;

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex1Settings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx1Settings.MaxReplicationQueueMemorySize = 10000;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
            publicReplicatorEx1Settings.SecondaryClearAcknowledgedOperations = true;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
            publicReplicatorEx1Settings.MaxReplicationMessageSize = 9999;

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex2Settings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
            publicReplicatorEx2Settings.UseStreamFaultsAndEndOfStreamOperationAck = true;

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex3Settings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;

            publicReplicatorEx3Settings.InitialSecondaryReplicationQueueSize = 128;
            publicReplicatorEx3Settings.MaxSecondaryReplicationQueueSize = 256;
            publicReplicatorEx3Settings.MaxSecondaryReplicationQueueMemorySize = 1234;
            publicReplicatorEx3Settings.InitialPrimaryReplicationQueueSize = 64;
            publicReplicatorEx3Settings.MaxPrimaryReplicationQueueSize = 128;
            publicReplicatorEx3Settings.MaxPrimaryReplicationQueueMemorySize = 12345;
            publicReplicatorEx3Settings.PrimaryWaitForPendingQuorumsTimeoutMilliseconds = 987;

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex4Settings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX4 publicReplicatorEx4Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
            publicReplicatorEx3Settings.Reserved = &publicReplicatorEx4Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;

            publicReplicatorEx4Settings.ReplicatorListenAddress = L"listenAddress1";
            publicReplicatorEx4Settings.ReplicatorPublishAddress = L"publishAddress1";

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex1Ex2Settings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx1Settings.MaxReplicationQueueMemorySize = 256;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
            publicReplicatorEx2Settings.UseStreamFaultsAndEndOfStreamOperationAck = true;

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex1Ex3Settings");
            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;
            publicReplicatorEx3Settings.InitialSecondaryReplicationQueueSize = 128;
            publicReplicatorEx3Settings.MaxSecondaryReplicationQueueSize = 256;
            publicReplicatorEx3Settings.MaxSecondaryReplicationQueueMemorySize = 1234;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx1Settings.MaxReplicationQueueMemorySize = 256;
            publicReplicatorEx3Settings.MaxPrimaryReplicationQueueSize = 128;
            publicReplicatorEx3Settings.MaxPrimaryReplicationQueueMemorySize = 12345;
            publicReplicatorEx3Settings.PrimaryWaitForPendingQuorumsTimeoutMilliseconds = 987;

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "Ex1Ex4Settings");
            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX4 publicReplicatorEx4Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
            publicReplicatorEx3Settings.Reserved = &publicReplicatorEx4Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx1Settings.MaxReplicationQueueMemorySize = 256;
            
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
            publicReplicatorEx4Settings.ReplicatorListenAddress = L"listenAddress1";

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;
            publicReplicatorEx4Settings.ReplicatorPublishAddress = L"publishAddress1";

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }

        {
            Trace.WriteInfo(TestSource, "AllSettings");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX4 publicReplicatorEx4Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
            publicReplicatorEx3Settings.Reserved = &publicReplicatorEx4Settings;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
            publicReplicatorSettings.BatchAcknowledgementIntervalMilliseconds = 1;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialCopyQueueSize = 2;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialReplicationQueueSize = 2;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxCopyQueueSize = 4;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxReplicationQueueSize = 4;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_ADDRESS;
            publicReplicatorSettings.ReplicatorAddress = L"address1";

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK;
            publicReplicatorSettings.RequireServiceAck = true;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_RETRY_INTERVAL;
            publicReplicatorSettings.RetryIntervalMilliseconds = 7;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx1Settings.MaxReplicationQueueMemorySize = 2566;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
            publicReplicatorEx1Settings.SecondaryClearAcknowledgedOperations = true;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
            publicReplicatorEx1Settings.MaxReplicationMessageSize = 999;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
            publicReplicatorEx2Settings.UseStreamFaultsAndEndOfStreamOperationAck = true;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorEx3Settings.InitialSecondaryReplicationQueueSize = 128;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx3Settings.MaxSecondaryReplicationQueueSize = 256;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorEx3Settings.MaxSecondaryReplicationQueueMemorySize = 2561;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorEx3Settings.InitialPrimaryReplicationQueueSize = 32;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorEx3Settings.MaxPrimaryReplicationQueueSize = 64;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorEx3Settings.MaxPrimaryReplicationQueueMemorySize = 2345;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;
            publicReplicatorEx3Settings.PrimaryWaitForPendingQuorumsTimeoutMilliseconds = 87;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_LISTEN_ADDRESS;
            publicReplicatorEx4Settings.ReplicatorListenAddress = L"listenAddress1";

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PUBLISH_ADDRESS;
            publicReplicatorEx4Settings.ReplicatorPublishAddress = L"publishAddress1";

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsSuccess(), L"Unexpected return code");
            VERIFY_IS_TRUE(AreEqual(&publicReplicatorSettings, &(*uptr)), L"Replicator Settings don't match");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidPointers)
    {
        {
            Trace.WriteInfo(TestSource, "InvalidPointers1");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
            publicReplicatorSettings.Reserved = NULL;

            Random r;
            double d = r.NextDouble();
            if (d < 0.33)
            {
                publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            }
            else if (d < 0.67)
            {
                publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
            }
            else
            {
                publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
            }

            ReplicatorSettingsUPtr uptr;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            Trace.WriteInfo(TestSource, "InvalidPointers2");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
            publicReplicatorSettings.Reserved = NULL;

            ReplicatorSettingsUPtr uptr;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            Trace.WriteInfo(TestSource, "InvalidPointers3");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings;
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = NULL;

            ReplicatorSettingsUPtr uptr;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            Trace.WriteInfo(TestSource, "InvalidPointers4");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings;
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = NULL;

            ReplicatorSettingsUPtr uptr;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            Trace.WriteInfo(TestSource, "InvalidPointers5");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings;
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings;
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = NULL;

            ReplicatorSettingsUPtr uptr;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            Trace.WriteInfo(TestSource, "InvalidPointers6");

            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
            FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
            publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
            publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
            publicReplicatorEx3Settings.Reserved = NULL;

            ReplicatorSettingsUPtr uptr;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialCopyQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidAddress)
    {
        Trace.WriteInfo(TestSource, "InvalidAddress");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
        publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
        publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
        publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
        publicReplicatorEx3Settings.Reserved = NULL;

        ReplicatorSettingsUPtr uptr;
        publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_ADDRESS;
        publicReplicatorSettings.ReplicatorAddress = L"";
        auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
    }

    BOOST_AUTO_TEST_CASE(InvalidListenAddress)
    {
        Trace.WriteInfo(TestSource, "InvalidListenAddress");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX4 publicReplicatorEx4Settings = { 0 };
        publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
        publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
        publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
        publicReplicatorEx3Settings.Reserved = &publicReplicatorEx4Settings;
        publicReplicatorEx4Settings.Reserved = NULL;

        ReplicatorSettingsUPtr uptr;
        publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_ADDRESS;
        publicReplicatorEx4Settings.ReplicatorListenAddress = L"";
        auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
    }

    BOOST_AUTO_TEST_CASE(InvalidPublishAddress)
    {
        Trace.WriteInfo(TestSource, "InvalidPublishAddress");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorEx1Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorEx2Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorEx3Settings = { 0 };
        FABRIC_REPLICATOR_SETTINGS_EX4 publicReplicatorEx4Settings = { 0 };
        publicReplicatorSettings.Reserved = &publicReplicatorEx1Settings;
        publicReplicatorEx1Settings.Reserved = &publicReplicatorEx2Settings;
        publicReplicatorEx2Settings.Reserved = &publicReplicatorEx3Settings;
        publicReplicatorEx3Settings.Reserved = &publicReplicatorEx4Settings;
        publicReplicatorEx4Settings.Reserved = NULL;

        ReplicatorSettingsUPtr uptr;
        publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_ADDRESS;
        publicReplicatorEx4Settings.ReplicatorPublishAddress = L"";
        auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);

        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
    }

    BOOST_AUTO_TEST_CASE(InvalidCopyQueueSize)
    {
        Trace.WriteInfo(TestSource, "InvalidCopyQueueSize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialCopyQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialCopyQueueSize = 0;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialCopyQueueSize = 123;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxCopyQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxCopyQueueSize = 123;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.MaxCopyQueueSize = 256;
            publicReplicatorSettings.InitialCopyQueueSize = 512;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.MaxCopyQueueSize = 256;
            publicReplicatorSettings.InitialCopyQueueSize = 256;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidReplicationQueueSize)
    {
        Trace.WriteInfo(TestSource, "InvalidReplicationQueueSize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialReplicationQueueSize = 0;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.InitialReplicationQueueSize = 123;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxReplicationQueueSize = 123;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettings.MaxReplicationQueueSize = 128;
            publicReplicatorSettings.InitialReplicationQueueSize = 512;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidReplicationQueueMemorySize)
    {
        Trace.WriteInfo(TestSource, "InvalidReplicationQueueMemorySize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorSettingsEx1;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE;

            publicReplicatorSettings.MaxReplicationQueueSize = 0;
            publicReplicatorSettingsEx1.MaxReplicationQueueMemorySize = 0;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;

            publicReplicatorSettingsEx1.MaxReplicationQueueMemorySize = 123;
            publicReplicatorSettingsEx1.MaxReplicationMessageSize = 1234;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidBatchAckRetry)
    {
        Trace.WriteInfo(TestSource, "InvalidBatchAckRetry");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        ReplicatorSettingsUPtr uptr;

        publicReplicatorSettings = { 0 };
        publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
        publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_RETRY_INTERVAL;

        publicReplicatorSettings.BatchAcknowledgementIntervalMilliseconds = 1234;
        publicReplicatorSettings.RetryIntervalMilliseconds = 123;

        auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
    }

    BOOST_AUTO_TEST_CASE(InvalidPrimaryReplicationQueueSize)
    {
        Trace.WriteInfo(TestSource, "InvalidPrimaryReplicationQueueSize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorSettingsEx1;
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorSettingsEx2;
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorSettingsEx3;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettingsEx3.InitialPrimaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettingsEx3.InitialPrimaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettingsEx3.InitialPrimaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettingsEx3.MaxPrimaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettingsEx3.MaxPrimaryReplicationQueueSize = 123;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettingsEx3.MaxPrimaryReplicationQueueSize = 128;
            publicReplicatorSettingsEx3.InitialPrimaryReplicationQueueSize = 512;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidPrimaryReplicationQueueMemorySize)
    {
        Trace.WriteInfo(TestSource, "InvalidPrimaryReplicationQueueMemorySize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorSettingsEx1;
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorSettingsEx2;
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorSettingsEx3;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;

            publicReplicatorSettingsEx3.MaxPrimaryReplicationQueueMemorySize = 0;
            publicReplicatorSettingsEx3.MaxPrimaryReplicationQueueSize = 0;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;

            publicReplicatorSettingsEx1.MaxReplicationMessageSize = 123;
            publicReplicatorSettingsEx3.MaxPrimaryReplicationQueueMemorySize = 13;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidSecondaryReplicationQueueSize)
    {
        Trace.WriteInfo(TestSource, "InvalidSecondaryReplicationQueueSize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorSettingsEx1;
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorSettingsEx2;
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorSettingsEx3;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettingsEx3.InitialSecondaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettingsEx3.InitialSecondaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettingsEx3.InitialSecondaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettingsEx3.MaxSecondaryReplicationQueueSize = 1;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettingsEx3.MaxSecondaryReplicationQueueSize = 123;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            publicReplicatorSettingsEx3.MaxSecondaryReplicationQueueSize = 128;
            publicReplicatorSettingsEx3.InitialSecondaryReplicationQueueSize = 512;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_CASE(InvalidSecondaryReplicationQueueMemorySize)
    {
        Trace.WriteInfo(TestSource, "InvalidSecondaryReplicationQueueMemorySize");

        FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings;
        FABRIC_REPLICATOR_SETTINGS_EX1 publicReplicatorSettingsEx1;
        FABRIC_REPLICATOR_SETTINGS_EX2 publicReplicatorSettingsEx2;
        FABRIC_REPLICATOR_SETTINGS_EX3 publicReplicatorSettingsEx3;
        ReplicatorSettingsUPtr uptr;

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;

            publicReplicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize = 0;
            publicReplicatorSettingsEx3.MaxSecondaryReplicationQueueSize = 0;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }

        {
            uptr = nullptr;
            publicReplicatorSettings = { 0 };
            publicReplicatorSettingsEx1 = { 0 };
            publicReplicatorSettingsEx2 = { 0 };
            publicReplicatorSettingsEx3 = { 0 };
            publicReplicatorSettings.Reserved = &publicReplicatorSettingsEx1;
            publicReplicatorSettingsEx1.Reserved = &publicReplicatorSettingsEx2;
            publicReplicatorSettingsEx2.Reserved = &publicReplicatorSettingsEx3;
            publicReplicatorSettingsEx3.Reserved = NULL;

            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            publicReplicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;

            publicReplicatorSettingsEx1.MaxReplicationMessageSize = 123;
            publicReplicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize = 13;
            auto error = ReplicatorSettings::FromPublicApi(publicReplicatorSettings, uptr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument), L"Unexpected return code");
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool ReplicatorSettingsTest::AreEqual(FABRIC_REPLICATOR_SETTINGS *original, ReplicatorSettings *output)
    {
        ScopedHeap heap;
        FABRIC_REPLICATOR_SETTINGS generated = { 0 };
        output->ToPublicApi(heap, generated);

        VERIFY_ARE_EQUAL(original->Flags, generated.Flags);
        VERIFY_ARE_EQUAL(original->BatchAcknowledgementIntervalMilliseconds, generated.BatchAcknowledgementIntervalMilliseconds);
        VERIFY_ARE_EQUAL(original->InitialCopyQueueSize, generated.InitialCopyQueueSize);
        VERIFY_ARE_EQUAL(original->InitialReplicationQueueSize, generated.InitialReplicationQueueSize);
        VERIFY_ARE_EQUAL(original->MaxCopyQueueSize, generated.MaxCopyQueueSize);
        VERIFY_ARE_EQUAL(original->MaxReplicationQueueSize, generated.MaxReplicationQueueSize);
        VERIFY_ARE_EQUAL(original->RequireServiceAck, generated.RequireServiceAck);
        VERIFY_ARE_EQUAL(original->RetryIntervalMilliseconds, generated.RetryIntervalMilliseconds);

        if (original->Flags & FABRIC_REPLICATOR_ADDRESS)
        {
            // Do this check only if there is a valid pointer
            VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(original->ReplicatorAddress, generated.ReplicatorAddress));
        }

        // verify Ex1 settings
        if ((original->Flags & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS) ||
            (original->Flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE))
        {
            FABRIC_REPLICATOR_SETTINGS_EX1 * originalEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(original->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX1 * generatedEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(generated.Reserved);

            VERIFY_ARE_EQUAL(originalEx1->MaxReplicationMessageSize, generatedEx1->MaxReplicationMessageSize);
            VERIFY_ARE_EQUAL(originalEx1->MaxReplicationQueueMemorySize, generatedEx1->MaxReplicationQueueMemorySize);
            VERIFY_ARE_EQUAL(originalEx1->SecondaryClearAcknowledgedOperations, generatedEx1->SecondaryClearAcknowledgedOperations);
        }

        // verify Ex2 settings
        if ((original->Flags & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK))
        {
            FABRIC_REPLICATOR_SETTINGS_EX1 * originalEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(original->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX1 * generatedEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(generated.Reserved);

            FABRIC_REPLICATOR_SETTINGS_EX2 * originalEx2 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(originalEx1->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX2 * generatedEx2 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(generatedEx1->Reserved);

            VERIFY_ARE_EQUAL(originalEx2->UseStreamFaultsAndEndOfStreamOperationAck, generatedEx2->UseStreamFaultsAndEndOfStreamOperationAck);
        }

        // verify Ex3 settings
        if ((original->Flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) ||
            (original->Flags & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT))
        {
            FABRIC_REPLICATOR_SETTINGS_EX1 * originalEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(original->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX1 * generatedEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(generated.Reserved);

            FABRIC_REPLICATOR_SETTINGS_EX2 * originalEx2 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(originalEx1->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX2 * generatedEx2 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(generatedEx1->Reserved);

            FABRIC_REPLICATOR_SETTINGS_EX3 * originalEx3 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX3*>(originalEx2->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX3 * generatedEx3 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX3*>(generatedEx2->Reserved);

            VERIFY_ARE_EQUAL(originalEx3->InitialSecondaryReplicationQueueSize, generatedEx3->InitialSecondaryReplicationQueueSize);
            VERIFY_ARE_EQUAL(originalEx3->MaxSecondaryReplicationQueueSize, generatedEx3->MaxSecondaryReplicationQueueSize);
            VERIFY_ARE_EQUAL(originalEx3->MaxSecondaryReplicationQueueMemorySize, generatedEx3->MaxSecondaryReplicationQueueMemorySize);
            VERIFY_ARE_EQUAL(originalEx3->InitialPrimaryReplicationQueueSize, generatedEx3->InitialPrimaryReplicationQueueSize);
            VERIFY_ARE_EQUAL(originalEx3->MaxPrimaryReplicationQueueSize, generatedEx3->MaxPrimaryReplicationQueueSize);
            VERIFY_ARE_EQUAL(originalEx3->MaxPrimaryReplicationQueueMemorySize, generatedEx3->MaxPrimaryReplicationQueueMemorySize);
            VERIFY_ARE_EQUAL(originalEx3->PrimaryWaitForPendingQuorumsTimeoutMilliseconds, generatedEx3->PrimaryWaitForPendingQuorumsTimeoutMilliseconds);
        }

        // verify Ex4 settings
        if ((original->Flags & FABRIC_REPLICATOR_LISTEN_ADDRESS) ||
            (original->Flags & FABRIC_REPLICATOR_PUBLISH_ADDRESS))
        {
            FABRIC_REPLICATOR_SETTINGS_EX1 * originalEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(original->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX1 * generatedEx1 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(generated.Reserved);

            FABRIC_REPLICATOR_SETTINGS_EX2 * originalEx2 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(originalEx1->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX2 * generatedEx2 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(generatedEx1->Reserved);

            FABRIC_REPLICATOR_SETTINGS_EX3 * originalEx3 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX3*>(originalEx2->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX3 * generatedEx3 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX3*>(generatedEx2->Reserved);

            FABRIC_REPLICATOR_SETTINGS_EX4 * originalEx4 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX4*>(originalEx3->Reserved);
            FABRIC_REPLICATOR_SETTINGS_EX4 * generatedEx4 = static_cast<FABRIC_REPLICATOR_SETTINGS_EX4*>(generatedEx3->Reserved);

            VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(originalEx4->ReplicatorListenAddress, generatedEx4->ReplicatorListenAddress));
            VERIFY_IS_TRUE(Common::StringUtility::AreEqualCaseInsensitive(originalEx4->ReplicatorPublishAddress, generatedEx4->ReplicatorPublishAddress));
        }

        return true;
    }

}
