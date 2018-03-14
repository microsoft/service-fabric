// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{

    // Test health client
    class TestHealthClient : public Reliability::ReplicationComponent::IReplicatorHealthClient
    {
    public:
        TestHealthClient()
        {
        }

        Common::ErrorCode ReportReplicatorHealth(
            Common::SystemHealthReportCode::Enum reportCode,
            wstring const & dynamicProperty,
            wstring const & extraDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            Common::TimeSpan const & timeToLive)
        {
            UNREFERENCED_PARAMETER(reportCode);
            UNREFERENCED_PARAMETER(dynamicProperty);
            UNREFERENCED_PARAMETER(extraDescription);
            UNREFERENCED_PARAMETER(sequenceNumber);
            UNREFERENCED_PARAMETER(timeToLive);

            return Common::ErrorCode();
        }

        static shared_ptr<TestHealthClient> Create()
        {
            return make_shared<TestHealthClient>();
        }
    };

    typedef std::shared_ptr<TestHealthClient> TestHealthClientSPtr;
}
