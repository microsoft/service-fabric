// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/macro.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class IEventWriter
            {
                DENY_COPY(IEventWriter);
            public:    
                IEventWriter() = default;
                virtual ~IEventWriter() {};
                virtual void Trace(IEventData const &&) = 0;
            };
        }
    }
}
