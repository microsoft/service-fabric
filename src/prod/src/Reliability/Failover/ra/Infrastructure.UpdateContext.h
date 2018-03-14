// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class UpdateContext
            {
            public:
                virtual ~UpdateContext() {}

                virtual void EnableUpdate() = 0;

                virtual void EnableInMemoryUpdate() = 0;
            };
        }
    }
}



