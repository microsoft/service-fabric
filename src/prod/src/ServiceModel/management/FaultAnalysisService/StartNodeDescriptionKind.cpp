// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace StartNodeDescriptionKind
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case UsingNodeName:
                    w << L"UsingNodeName";
                    return;
                case UsingReplicaSelector:
                    w << L"UsingReplicaSelector";
                    return;
                default:
                    w << Common::wformatString("Unknown StartNodeDescriptionKind value {0}", static_cast<int>(val));
                }
            }

            Enum FromPublicApi(FABRIC_START_NODE_DESCRIPTION_KIND const & kind)
            {
                switch (kind)
                {
                case FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME:
                    return UsingNodeName;

                case FABRIC_START_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR:
                    return UsingReplicaSelector;
                default:
                    return None;
                }
            }

            FABRIC_START_NODE_DESCRIPTION_KIND ToPublicApi(Enum const & kind)
            {
                switch (kind)
                {
                case UsingNodeName:
                    return FABRIC_START_NODE_DESCRIPTION_KIND_USING_NODE_NAME;

                case UsingReplicaSelector:
                    return FABRIC_START_NODE_DESCRIPTION_KIND_USING_REPLICA_SELECTOR;

                default:
                    return FABRIC_START_NODE_DESCRIPTION_KIND_INVALID;
                }
            }
        }
    }
}
	
