// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability {
namespace ReplicationComponent {
namespace HealthReportType
{
    enum Enum
    {
        PrimaryReplicationQueueStatus = 0,
        SecondaryReplicationQueueStatus = 1,
        RemoteReplicatorConnectionStatus = 2,

        LastValidEnum = RemoteReplicatorConnectionStatus
    };

    static const int ttlForOkReportInSeconds = 300;

    void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

    std::wstring GetHealthPropertyName(Enum const & reportType);

    std::wstring GetHealthPropertyName(
        Common::ApiMonitoring::ApiName::Enum const & apiName, 
        FABRIC_REPLICA_ID const & targetReplicaId);

    Common::SystemHealthReportCode::Enum GetHealthReportCode(
        ::FABRIC_HEALTH_STATE toReport,
        Common::ApiMonitoring::ApiName::Enum apiName);

    Common::SystemHealthReportCode::Enum GetHealthReportCode(
        ::FABRIC_HEALTH_STATE toReport,
        Enum const & reportType);

    Common::TimeSpan GetHealthReportTTL(
        ::FABRIC_HEALTH_STATE toReport,
        Enum const & reportType);

    Common::TimeSpan GetHealthReportTTL(
        ::FABRIC_HEALTH_STATE toReport,
        Common::ApiMonitoring::ApiName::Enum apiName);

    DECLARE_ENUM_STRUCTURED_TRACE(HealthReportType);
}
}
}
