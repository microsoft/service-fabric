// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            class NodeDeactivationInfo
            {
            public:
                NodeDeactivationInfo() :
                    isActivated_(true),
                    sequenceNumber_(Constants::InvalidNodeActivationSequenceNumber)
                {
                }

                NodeDeactivationInfo(bool isActivated, int64 sequenceNumber) :
                    isActivated_(isActivated),
                    sequenceNumber_(sequenceNumber)
                {
                }

                __declspec(property(get = get_IsActivated)) bool IsActivated;
                bool get_IsActivated() const { return isActivated_; }

                __declspec(property(get = get_SequenceNumber)) int64 SequenceNumber;
                int64 get_SequenceNumber() const { return sequenceNumber_; }

                static std::string AddField(Common::TraceEvent& traceEvent, std::string const& name);
                void FillEventData(Common::TraceEventContext& context) const;
                void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;

            private:
                bool isActivated_;
                int64 sequenceNumber_;
            };
        }
    }
}



