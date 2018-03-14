// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::TextWriter;
using std::wstring;

namespace HealthReportType
{
void WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case PrimaryReplicationQueueStatus:
            w << L"PrimaryReplicationQueueStatus"; return;
        case SecondaryReplicationQueueStatus:
            w << L"SecondaryReplicationQueueStatus"; return;
        case RemoteReplicatorConnectionStatus:
            w << L"RemoteReplicatorConnectionStatus"; return;
        default: 
            Common::Assert::CodingError("Unknown HealthReportType:{0} in Replicator", val);
    }
}

ENUM_STRUCTURED_TRACE(HealthReportType, PrimaryReplicationQueueStatus, LastValidEnum);

wstring GetHealthPropertyName(
    Common::ApiMonitoring::ApiName::Enum const & apiName,
    FABRIC_REPLICA_ID const & targetReplicaId)
{
    switch (apiName)
    {
        case Common::ApiMonitoring::ApiName::GetNextCopyState:
            return Common::wformatString("GetNextCopyState_For_{0}", targetReplicaId);
        case Common::ApiMonitoring::ApiName::GetNextCopyContext:
            return L"GetNextCopyContext";
        default:
            Common::Assert::CodingError("Unknown ApiName:{0} in Replicator", apiName);
    }
}

wstring GetHealthPropertyName(Enum const & reportType)
{
    switch (reportType)
    {
        case PrimaryReplicationQueueStatus:
            return L"PrimaryReplicationQueueStatus";
        case SecondaryReplicationQueueStatus:
            return L"SecondaryReplicationQueueStatus";
        case RemoteReplicatorConnectionStatus:
            return L"RemoteReplicatorConnectionStatus";
        default:
            Common::Assert::CodingError("Unknown HealthReportType:{0} in Replicator", reportType);
    }
}

Common::SystemHealthReportCode::Enum GetHealthReportCode(
    ::FABRIC_HEALTH_STATE toReport,
    Common::ApiMonitoring::ApiName::Enum apiName)
{
    switch (apiName)
    {
        case Common::ApiMonitoring::ApiName::GetNextCopyState:
        case Common::ApiMonitoring::ApiName::GetNextCopyContext:
            switch (toReport)
            {
                case ::FABRIC_HEALTH_STATE_OK:
                    return Common::SystemHealthReportCode::RE_ApiOk;
                case ::FABRIC_HEALTH_STATE_WARNING:
                    return Common::SystemHealthReportCode::RE_ApiSlow;
                default:
                    Common::Assert::CodingError("HealthReportType.GetHealthReportCode - Unknown Replicator Health State {0}", toReport);
            }
            break;
        default:
            Common::Assert::CodingError("Unknown ReplicatorApi:{0} in Replicator", apiName);
    }
}

Common::SystemHealthReportCode::Enum GetHealthReportCode(
    ::FABRIC_HEALTH_STATE toReport,
    Enum const & reportType)
{
    switch (reportType)
    {
        case PrimaryReplicationQueueStatus:
        case SecondaryReplicationQueueStatus:
            switch (toReport)
            {
                case ::FABRIC_HEALTH_STATE_OK:
                    return Common::SystemHealthReportCode::RE_QueueOk;
                case ::FABRIC_HEALTH_STATE_WARNING:
                    return Common::SystemHealthReportCode::RE_QueueFull;
                default:
                    Common::Assert::CodingError("HealthReportType.GetHealthReportCode - Unknown Replicator Health State {0}", toReport);
            }
            break;
        case RemoteReplicatorConnectionStatus:
            switch (toReport)
            {
                case ::FABRIC_HEALTH_STATE_OK:
                    return Common::SystemHealthReportCode::RE_RemoteReplicatorConnectionStatusOk;
                case ::FABRIC_HEALTH_STATE_WARNING:
                    return Common::SystemHealthReportCode::RE_RemoteReplicatorConnectionStatusFailed;
                default:
                    Common::Assert::CodingError("HealthReportType.GetHealthReportCode - Unknown Replicator Health State {0}", toReport);
            }
            break;
        default:
            Common::Assert::CodingError("Unknown HealthReportType:{0} in Replicator", reportType);
    }
}

Common::TimeSpan GetHealthReportTTL(
    ::FABRIC_HEALTH_STATE toReport,
    Enum const & reportType)
{
    switch (reportType)
    {
        case PrimaryReplicationQueueStatus:
        case SecondaryReplicationQueueStatus:
            switch (toReport)
            {
                case ::FABRIC_HEALTH_STATE_OK:
                    return Common::TimeSpan::FromSeconds(ttlForOkReportInSeconds);
                case ::FABRIC_HEALTH_STATE_WARNING:
                    return Common::TimeSpan::MaxValue;
                default:
                    Common::Assert::CodingError("HealthReportType.GetHealthReportTTL - Unknown Replicator Health State {0}", toReport);
            }
            break;
        case RemoteReplicatorConnectionStatus:
            switch (toReport)
            {
                case ::FABRIC_HEALTH_STATE_OK:
                    return Common::TimeSpan::FromSeconds(ttlForOkReportInSeconds);
                case ::FABRIC_HEALTH_STATE_WARNING:
                    return Common::TimeSpan::MaxValue;
                default:
                    Common::Assert::CodingError("HealthReportType.GetHealthReportTTL - Unknown Replicator Health State {0}", toReport);
            }
            break;
        default:
            Common::Assert::CodingError("Unknown HealthReportType:{0} in Replicator", reportType);
    }
}

Common::TimeSpan GetHealthReportTTL(
    ::FABRIC_HEALTH_STATE toReport,
    Common::ApiMonitoring::ApiName::Enum apiName)
{
    switch (apiName)
    {
        case Common::ApiMonitoring::ApiName::GetNextCopyState:
        case Common::ApiMonitoring::ApiName::GetNextCopyContext:
            switch (toReport)
            {
                case ::FABRIC_HEALTH_STATE_OK:
                    return Common::TimeSpan::FromSeconds(ttlForOkReportInSeconds);
                case ::FABRIC_HEALTH_STATE_WARNING:
                    return Common::TimeSpan::MaxValue;
                default:
                    Common::Assert::CodingError("HealthReportType.GetHealthReportTTL - Unknown Replicator Health State {0}", toReport);
            }
            break;
        default:
            Common::Assert::CodingError("Unknown ReplicatorApi:{0} in Replicator", apiName);
    }

}

}

} // end namespace ReplicationComponent
} // end namespace Reliability
