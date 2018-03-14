// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class IReplicatorHealthClient
        {
        public:
            virtual Common::ErrorCode ReportReplicatorHealth(
                Common::SystemHealthReportCode::Enum reportCode,
                std::wstring const & dynamicProperty,
                std::wstring const & extraDescription,
                FABRIC_SEQUENCE_NUMBER sequenceNumber,
                Common::TimeSpan const & timeToLive) = 0;
        };
    }
}
