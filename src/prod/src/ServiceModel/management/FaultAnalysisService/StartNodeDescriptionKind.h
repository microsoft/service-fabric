// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace FaultAnalysisService
    {
        namespace StartNodeDescriptionKind
        {
            enum Enum
            {
                None = 0,
                UsingNodeName = 1,
                UsingReplicaSelector = 2,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            Enum FromPublicApi(FABRIC_START_NODE_DESCRIPTION_KIND const &);
            FABRIC_START_NODE_DESCRIPTION_KIND ToPublicApi(Enum const &);
        };
    }
}
