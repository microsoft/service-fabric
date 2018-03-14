// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FailoverUnitReconfigurationStage
        {
            void FailoverUnitReconfigurationStage::WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Phase0_Demote:
                    w << "Phase0_Demote"; return;
                case Phase1_GetLSN:
                    w << L"Phase1_GetLSN"; return;
                case Phase2_Catchup:
                    w << L"Phase2_Catchup"; return;
                case Phase3_Deactivate: 
                    w << L"Phase3_Deactivate"; return;
                case Phase4_Activate: 
                    w << L"Phase4_Activate"; return;
                case None: 
                    w << L"None"; return;
                case Abort_Phase0_Demote:
                    w << L"Abort_Phase0_Demote"; return;
                default: 
                    Common::Assert::CodingError("Unknown FailoverUnit Reconfiguration Stage {0}", static_cast<int>(val));
                }
            }

            ENUM_STRUCTURED_TRACE(FailoverUnitReconfigurationStage, Phase0_Demote, LastValidEnum);

            ::FABRIC_RECONFIGURATION_PHASE Reliability::ReconfigurationAgentComponent::FailoverUnitReconfigurationStage::ConvertToPublicReconfigurationPhase(FailoverUnitReconfigurationStage::Enum toConvert)
            {
                switch (toConvert)
                {
                case None:
                    return ::FABRIC_RECONFIGURATION_PHASE_NONE;
                case Phase0_Demote:
                    return ::FABRIC_RECONFIGURATION_PHASE_ZERO;
                case Phase1_GetLSN:
                    return ::FABRIC_RECONFIGURATION_PHASE_ONE;
                case Phase2_Catchup:
                    return ::FABRIC_RECONFIGURATION_PHASE_TWO;
                case Phase3_Deactivate:
                    return ::FABRIC_RECONFIGURATION_PHASE_THREE;
                case Phase4_Activate:
                    return ::FABRIC_RECONFIGURATION_PHASE_FOUR;
                case Abort_Phase0_Demote:
                    return ::FABRIC_RECONFIGURATION_ABORT_PHASE_ZERO;
                default:
                    Common::Assert::CodingError("Unknown Reconfiguration Phase");
                }
            }
        }
    }
}
