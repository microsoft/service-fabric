// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        #define FAS_RESOURCE( ResourceProperty, ResourceName ) \
            DEFINE_STRING_RESOURCE(ManagementResource, ResourceProperty, FAS, ResourceName) \

        class FASResource
        {
            DECLARE_SINGLETON_RESOURCE(FASResource)

            FAS_RESOURCE(DataLossModeInvalid, DataLossModeInvalid)
            FAS_RESOURCE(DataLossModeUnknown, DataLossModeUnknown)
            FAS_RESOURCE(QuorumLossModeInvalid, QuorumLossModeInvalid)
            FAS_RESOURCE(QuorumLossModeUnknown, QuorumLossModeUnknown)
            FAS_RESOURCE(RestartPartitionModeInvalid, RestartPartitionModeInvalid)
            FAS_RESOURCE(RestartPartitionModeUnknown, RestartPartitionModeUnknown)
            FAS_RESOURCE(ChaosStatusUnknown, ChaosStatusUnknown)
            FAS_RESOURCE(ChaosTimeToRunTooBig, ChaosTimeToRunTooBig)
            FAS_RESOURCE(ChaosContextNullKeyOrValue, ChaosContextNullKeyOrValue)
            FAS_RESOURCE(ChaosNodeTypeInclusionListTooLong, ChaosNodeTypeInclusionListTooLong)
            FAS_RESOURCE(ChaosApplicationInclusionListTooLong, ChaosApplicationInclusionListTooLong)
            FAS_RESOURCE(ChaosTargetFilterSpecificationNotFound, ChaosTargetFilterSpecificationNotFound)
            FAS_RESOURCE(ChaosScheduleStatusUnknown, ChaosScheduleStatusUnknown)
        };
    }
}
