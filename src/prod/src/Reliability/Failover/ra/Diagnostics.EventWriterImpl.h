// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Diagnostics.IEventWriter.h"
#include "Common/macro.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class EventWriterImpl : public IEventWriter
            {
            public:
                DENY_COPY(EventWriterImpl);
                ~EventWriterImpl() {};

                EventWriterImpl() = default;

                EventWriterImpl(std::wstring const & nodeName)
                    : nodeName_(nodeName)
                {}

                void Trace(Diagnostics::IEventData const && eventData) override;

                __declspec (property(get=get_NodeName)) std::wstring const & NodeName;
                std::wstring const & get_NodeName() const { return nodeName_; } ;

            private:
                std::wstring nodeName_;
            };
        }
    }
}
