// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Health
        {
            struct HealthContext
            {
            public:
                HealthContext(std::wstring nodeName) :
                    NodeName(nodeName)
                {}
                std::wstring const NodeName;
            };

        }
    }
}
