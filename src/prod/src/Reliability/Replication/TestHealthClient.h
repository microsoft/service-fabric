// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    class HealthClient;
    typedef std::shared_ptr<HealthClient> HealthClientSPtr;

    class TestHealthClientReportEntry {
    public:
        TestHealthClientReportEntry(
            SystemHealthReportCode::Enum reportCode,
            wstring const & dynamicProperty,
            wstring const & extraDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            TimeSpan const & timeToLive)
            : reportCode_(reportCode),
            dynamicProperty_(dynamicProperty),
            extraDescription_(extraDescription),
            sequenceNumber_(sequenceNumber),
            timeToLive_(timeToLive)
        {
        }

        bool operator==(const TestHealthClientReportEntry& other)
        {
            return reportCode_ == other.reportCode_;
        }

    private:
        SystemHealthReportCode::Enum reportCode_;
        wstring dynamicProperty_;
        wstring extraDescription_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        TimeSpan const & timeToLive_;
    };

    class HealthClient : public IReplicatorHealthClient
    {
    public:
        HealthClient(bool expectReports)
            : expectReports_(expectReports)
            , numberOfReportsSent_(0)
        {
        }

        ErrorCode ReportReplicatorHealth(
            SystemHealthReportCode::Enum reportCode,
            wstring const & dynamicProperty,
            wstring const & extraDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            TimeSpan const & timeToLive)
        {
            UNREFERENCED_PARAMETER(reportCode);
            UNREFERENCED_PARAMETER(dynamicProperty);
            UNREFERENCED_PARAMETER(extraDescription);
            UNREFERENCED_PARAMETER(sequenceNumber);
            UNREFERENCED_PARAMETER(timeToLive);

            numberOfReportsSent_ += 1;

            if (!expectReports_)
            {
                VERIFY_FAIL(L"No health report expected");
            }
            else
            {
                reports_.push_back(TestHealthClientReportEntry(
                    reportCode,
                    dynamicProperty,
                    extraDescription,
                    sequenceNumber,
                    timeToLive
                ));
            }

            return ErrorCode();
        }

        __declspec(property(get = get_NumberofReportsSent)) int NumberofReportsSent;
        int get_NumberofReportsSent() const
        {
            return numberOfReportsSent_;
        }

        static HealthClientSPtr Create(bool expectReports)
        {
            return make_shared<HealthClient>(expectReports);
        }

        bool IsReported(const TestHealthClientReportEntry & reportEntry)
        {
            return std::find(std::begin(reports_) , std::end(reports_), reportEntry) != std::end(reports_);
        }

    private:
        bool expectReports_;
        int numberOfReportsSent_;
        std::vector<TestHealthClientReportEntry> reports_;
    };
}
