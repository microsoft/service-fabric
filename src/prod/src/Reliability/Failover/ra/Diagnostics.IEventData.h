// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Diagnostics.TraceEventType.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            // This interface is the base for classes that hold information
            // for trace events. Classes that inherit from this class for ft 
            // should be placed at Diagnostics.FailoverUnitEventData and have
            // a descriptive type added to the TraceEventType enum.
            // The specilized classes will be members of the
            // TraceEventStateMachineAction template.
            class IEventData
            {
            public:                
                virtual ~IEventData() {};
                IEventData(TraceEventType::Enum traceEventType) :
                    traceEventType_(traceEventType)
                {
                }

                __declspec(property(get = get_TraceEventType)) TraceEventType::Enum TraceEventType;
                TraceEventType::Enum get_TraceEventType() const { return traceEventType_; }

                // Clone is a pure virtual method used by the UnitTests to 
                // access a copy of the data that will expire when the Action 
                // completes and is destroyed
                virtual std::shared_ptr<IEventData> Clone() const = 0;

            protected:
                IEventData(const IEventData &other) :
                    traceEventType_(other.traceEventType_)
                {
                }
                IEventData& operator=(const IEventData &other)
                {
                    traceEventType_ = other.traceEventType_;
                    return *this;
                }

            private:
                TraceEventType::Enum traceEventType_;
            };
        }
    }
}
