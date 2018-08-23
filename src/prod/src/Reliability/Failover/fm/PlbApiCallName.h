// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace PlbApiCallName
        {
            enum Enum
            {
                SetMovementEnabled = 0,
                UpdateNode = 1,
                UpdateServiceType = 2,
                DeleteServiceType = 3,
                UpdateApplication = 4,
                DeleteApplication = 5,
                UpdateService = 6,
                DeleteService = 7,
                UpdateFailoverUnit = 8,
                DeleteFailoverUnit = 9,
                UpdateLoadOrMoveCost = 10,
                ResetPartitionLoad = 11,
                CompareNodeForPromotion = 12,
                Dispose = 13,
                GetClusterLoadInformationQueryResult = 14,
                GetNodeLoadInformationQueryResult = 15,
                GetUnbalancedReplicaInformationQueryResult = 16,
                GetApplicationLoadInformationResult = 17,
                TriggerPromoteToPrimary = 18,
                TriggerSwapPrimary = 19,
                TriggerMoveSecondary = 20,
                OnDroppedPLBMovement = 21,
                ToggleVerboseServicePlacementHealthReporting = 22,
                OnDroppedPLBMovements = 23,
                OnExecutePLBMovement = 24,
                UpdateClusterUpgrade = 25,
                OnFMBusy = 26,
                OnSafetyCheckAcknowledged = 27,
                UpdateAvailableImagesPerNode = 28,
                LastValidEnum = UpdateAvailableImagesPerNode
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(PlbApiCallName);
        };
    }
}
