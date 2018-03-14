// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TransactionalReplicatorTests
{
    // Test health client to v1 replicator
    class TestHealthClient : public Reliability::ReplicationComponent::IReplicatorHealthClient
    {
    public:
        TestHealthClient(bool expectReports)
            : expectReports_(expectReports)
            , numberOfReportsSent_(0)
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

            numberOfReportsSent_ += 1;

            if (!expectReports_)
            {
                ASSERT_IFNOT(false, "Unexpected health report in TestHealthClient");
            }

            return Common::ErrorCode();
        }

        __declspec(property(get = get_NumberofReportsSent)) int NumberofReportsSent;
        int get_NumberofReportsSent() const
        {
            return numberOfReportsSent_;
        }

        static shared_ptr<TestHealthClient> Create(bool expectReports)
        {
            return make_shared<TestHealthClient>(expectReports);
        }

    private:
        bool expectReports_;
        int numberOfReportsSent_;
    };

    typedef std::shared_ptr<TestHealthClient> TestHealthClientSPtr;

}
