// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace PlbApiCallName
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case SetMovementEnabled:
                    w << "SetMovementEnabled";
                    return;
                case UpdateNode:
                    w << "UpdateNode";
                    return;
                case UpdateServiceType:
                    w << "UpdateServiceType";
                    return;
                case DeleteServiceType:
                    w << "DeleteServiceType";
                    return;
                case UpdateApplication:
                    w << "UpdateApplication";
                    return;
                case DeleteApplication:
                    w << "DeleteApplication";
                    return;
                case UpdateService:
                    w << "UpdateService";
                    return;
                case DeleteService:
                    w << "DeleteService";
                    return;
                case UpdateFailoverUnit:
                    w << "UpdateFailoverUnit";
                    return;
                case DeleteFailoverUnit:
                    w << "DeleteFailoverUnit";
                    return;
                case UpdateLoadOrMoveCost:
                    w << "UpdateLoadOrMoveCost";
                    return;
                case ResetPartitionLoad:
                    w << "ResetPartitionLoad";
                    return;
                case CompareNodeForPromotion:
                    w << "CompareNodeForPromotion";
                    return;
                case Dispose:
                    w << "Dispose";
                    return;
                case GetClusterLoadInformationQueryResult:
                    w << "GetClusterLoadInformationQueryResult";
                    return;
                case GetNodeLoadInformationQueryResult:
                    w << "GetNodeLoadInformationQueryResult";
                    return;
                case GetUnbalancedReplicaInformationQueryResult:
                    w << "GetUnbalancedReplicaInformationQueryResult";
                    return;
                case GetApplicationLoadInformationResult:
                    w << "GetApplicationLoadInformationResult";
                    return;
                case TriggerPromoteToPrimary:
                    w << "TriggerPromoteToPrimary";
                    return;
                case TriggerSwapPrimary:
                    w << "TriggerSwapPrimary";
                    return;
                case TriggerMoveSecondary:
                    w << "TriggerMoveSecondary";
                    return;
                case OnDroppedPLBMovement:
                    w << "OnDroppedPLBMovement";
                    return;
                case ToggleVerboseServicePlacementHealthReporting:
                    w << "ToggleVerboseServicePlacementHealthReporting";
                    return;
                case OnDroppedPLBMovements:
                    w << "OnDroppedPLBMovements";
                    return;
                case OnExecutePLBMovement:
                    w << "OnExecutePLBMovement";
                    return;
                case UpdateClusterUpgrade:
                    w << "UpdateClusterUpgrade";
                    return;
                case OnFMBusy:
                    w << "OnFMBusy";
                    return;
                case OnSafetyCheckAcknowledged:
                    w << "OnSafetyCheckAcknowledged";
                    return;
                case UpdateAvailableImagesPerNode:
                    w << "UpdateAvailableImagesPerNode";
                    return;
                default:
                    Common::Assert::CodingError("Unknown {0}", static_cast<int>(val));
                }
            }

            ENUM_STRUCTURED_TRACE(PlbApiCallName, SetMovementEnabled, LastValidEnum);
        }
    }
}
