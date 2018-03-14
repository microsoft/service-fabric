// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <vector>
#include <memory>
#include "Reliability/Failover/ra/Diagnostics.IEventData.h"
#include "Reliability/Failover/ra/Diagnostics.IEventWriter.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class EventWriterStub : public IEventWriter
            {
            public:

                EventWriterStub() {};

                __declspec(property(get = get_TracedEvents)) std::vector<std::shared_ptr<IEventData>> & TracedEvents;
                std::vector<std::shared_ptr<IEventData>> & get_TracedEvents() { return tracedEvents_; }

                void Trace(IEventData const && eventData) override
                {
                    tracedEvents_.push_back(eventData.Clone());
                }

            private:
                std::vector<std::shared_ptr<IEventData>> tracedEvents_;

            };
        }
    }
}
