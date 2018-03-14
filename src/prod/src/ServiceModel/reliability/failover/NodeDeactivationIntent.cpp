// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability::NodeDeactivationIntent;

ErrorCode Reliability::NodeDeactivationIntent::FromPublic(
    FABRIC_NODE_DEACTIVATION_INTENT publicDeactivationIntent,
    __out Enum & deactivationIntent)
{
    ErrorCode error(ErrorCodeValue::Success);

    switch (publicDeactivationIntent)
    {
        case FABRIC_NODE_DEACTIVATION_INTENT_INVALID:
            error = ErrorCodeValue::InvalidArgument;
            break;
        case FABRIC_NODE_DEACTIVATION_INTENT_PAUSE:
            deactivationIntent = NodeDeactivationIntent::Pause;
            break;
        case FABRIC_NODE_DEACTIVATION_INTENT_RESTART:
            deactivationIntent = NodeDeactivationIntent::Restart;
            break;
        case FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA:
            deactivationIntent = NodeDeactivationIntent::RemoveData;
            break;
        case FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE:
            deactivationIntent = NodeDeactivationIntent::RemoveNode;
            break;
        default:
            error = ErrorCodeValue::InvalidArgument;
    }

    return error;
}

FABRIC_NODE_DEACTIVATION_INTENT Reliability::NodeDeactivationIntent::ToPublic(Enum deactivationIntent)
{
    switch (deactivationIntent)
    {
    case NodeDeactivationIntent::Pause:
        return FABRIC_NODE_DEACTIVATION_INTENT_PAUSE;
    case NodeDeactivationIntent::Restart:
        return FABRIC_NODE_DEACTIVATION_INTENT_RESTART;
    case NodeDeactivationIntent::RemoveData:
        return FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA;
    case NodeDeactivationIntent::RemoveNode:
        return FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE;
    default:
        return FABRIC_NODE_DEACTIVATION_INTENT_INVALID;
    }
}

void Reliability::NodeDeactivationIntent::WriteToTextWriter(TextWriter & w, Enum const& value)
{
    switch (value)
    {
        case None:
            w << L"None"; return;
        case Pause:
            w << L"Pause"; return;
        case Restart:
            w << L"Restart"; return;
        case RemoveData:
            w << L"RemoveData"; return;
        case RemoveNode:
            w << L"RemoveNode"; return;
        default:
            w << "NodeDeactivationIntent(" << static_cast<uint>(value) << ')'; return;
    }
}

BEGIN_ENUM_STRUCTURED_TRACE( NodeDeactivationIntent )

ADD_ENUM_MAP_VALUE( NodeDeactivationIntent, None )
ADD_ENUM_MAP_VALUE( NodeDeactivationIntent, Pause )
ADD_ENUM_MAP_VALUE( NodeDeactivationIntent, Restart )
ADD_ENUM_MAP_VALUE( NodeDeactivationIntent, RemoveData )
ADD_ENUM_MAP_VALUE( NodeDeactivationIntent, RemoveNode )

END_ENUM_STRUCTURED_TRACE( NodeDeactivationIntent )
