// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FailoverUnitReconfigurationStage
        {
            // Stages of FailoverUnit reconfiguration
            enum Enum
            {
                Phase0_Demote = 0,
                Phase1_GetLSN,
                Phase2_Catchup,
                Phase3_Deactivate,
                Phase4_Activate,
                None,
                Abort_Phase0_Demote,
                LastValidEnum = Abort_Phase0_Demote
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            ::FABRIC_RECONFIGURATION_PHASE ConvertToPublicReconfigurationPhase(FailoverUnitReconfigurationStage::Enum toConvert);

            DECLARE_ENUM_STRUCTURED_TRACE(FailoverUnitReconfigurationStage);
        }
    }
}
