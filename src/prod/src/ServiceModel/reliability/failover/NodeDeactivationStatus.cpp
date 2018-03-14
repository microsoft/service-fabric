// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::NodeDeactivationStatus;

FABRIC_NODE_DEACTIVATION_STATUS Reliability::NodeDeactivationStatus::ToPublic(Enum deactivationStatus)
{
    switch (deactivationStatus)
    {
    case DeactivationSafetyCheckInProgress:
        return FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_IN_PROGRESS;
    case DeactivationSafetyCheckComplete:
        return FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_COMPLETE;
    case DeactivationComplete:
        return FABRIC_NODE_DEACTIVATION_STATUS_COMPLETED;
    default:
        return FABRIC_NODE_DEACTIVATION_STATUS_NONE;
    }
}

void NodeDeactivationStatus::WriteToTextWriter(TextWriter & w, Enum const& value)
{
    switch (value)
    {
        case DeactivationSafetyCheckInProgress:
            w << L"DeactivationSafetyCheckInProgress"; return;
        case DeactivationSafetyCheckComplete:
            w << L"DeactivationSafetyCheckComplete"; return;
        case DeactivationComplete:
            w << L"DeactivationComplete"; return;
        case ActivationInProgress:
            w << L"ActivationInProgress"; return;
        case None:
            w << L"None"; return;
        default:
            w << "NodeDeactivationStatus(" << static_cast<uint>(value) << ')'; return;
    }
}

NodeDeactivationStatus::Enum NodeDeactivationStatus::Merge(
    NodeDeactivationStatus::Enum const & left,
    NodeDeactivationStatus::Enum const & right)
{
    // This depends on the ordering of the enum values, which describe
    // the progression of a deactivation workflow
    //
    return std::min(left, right);
}

BEGIN_ENUM_STRUCTURED_TRACE(NodeDeactivationStatus);

ADD_ENUM_MAP_VALUE(NodeDeactivationStatus, DeactivationSafetyCheckInProgress);
ADD_ENUM_MAP_VALUE(NodeDeactivationStatus, DeactivationSafetyCheckComplete);
ADD_ENUM_MAP_VALUE(NodeDeactivationStatus, DeactivationComplete);
ADD_ENUM_MAP_VALUE(NodeDeactivationStatus, ActivationInProgress);
ADD_ENUM_MAP_VALUE(NodeDeactivationStatus, None)

END_ENUM_STRUCTURED_TRACE(NodeDeactivationStatus);

