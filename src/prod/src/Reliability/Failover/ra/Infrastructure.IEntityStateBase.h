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
            // Base interface for all entity state
            class IEntityStateBase
            {
            public:
                virtual ~IEntityStateBase() {}

                virtual std::wstring ToString() const = 0;

            protected:
                IEntityStateBase() {}
            };
        }
    }
}
