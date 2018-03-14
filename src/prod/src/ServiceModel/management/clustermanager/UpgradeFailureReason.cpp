// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace UpgradeFailureReason
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case None: w << "None"; return;
                    case Interrupted: w << "Interrupted"; return;
                    case HealthCheck: w << "HealthCheck"; return;
                    case UpgradeDomainTimeout: w << "UpgradeDomainTimeout"; return;
                    case OverallUpgradeTimeout: w << "OverallUpgradeTimeout"; return;
                    case ProcessingFailure: w << "ProcessingFailure"; return;
                };

                w << "UpgradeFailureReason(" << static_cast<int>(e) << ')';
            }

            ENUM_STRUCTURED_TRACE( UpgradeFailureReason, None, LastValue );

            Enum FromPublicApi(FABRIC_UPGRADE_FAILURE_REASON publicType)
            {
                switch (publicType)
                {
                case FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED:
                    return Interrupted;

                case FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK:
                    return HealthCheck;

                case FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT:
                    return UpgradeDomainTimeout;

                case FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT:
                    return OverallUpgradeTimeout;

                case FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE:
                    return ProcessingFailure;

                case FABRIC_UPGRADE_FAILURE_REASON_NONE:
                default:
                    return None;
                }
            }

            FABRIC_UPGRADE_FAILURE_REASON ToPublicApi(Enum const nativeType)
            {
                switch (nativeType)
                {
                case Interrupted:
                    return FABRIC_UPGRADE_FAILURE_REASON_INTERRUPTED;

                case HealthCheck:
                    return FABRIC_UPGRADE_FAILURE_REASON_HEALTH_CHECK;

                case UpgradeDomainTimeout:
                    return FABRIC_UPGRADE_FAILURE_REASON_UPGRADE_DOMAIN_TIMEOUT;

                case OverallUpgradeTimeout:
                    return FABRIC_UPGRADE_FAILURE_REASON_OVERALL_UPGRADE_TIMEOUT;

                case ProcessingFailure:
                    return FABRIC_UPGRADE_FAILURE_REASON_PROCESSING_FAILURE;

                case None:
                default:
                    return FABRIC_UPGRADE_FAILURE_REASON_NONE;
                }
            }
        }
    }
}
