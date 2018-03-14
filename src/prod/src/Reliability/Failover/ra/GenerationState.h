// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class GenerationState
        {
        public:
            GenerationState()
                :   proposedGeneration_(Reliability::GenerationNumber::Zero),
                    receiveGeneration_(Reliability::GenerationNumber::Zero),
                    sendGeneration_(Reliability::GenerationNumber::Zero)
            {
            }

            __declspec (property(get=get_ProposedGeneration, put=set_ProposedGeneration)) Reliability::GenerationNumber const& ProposedGeneration;
            Reliability::GenerationNumber const& get_ProposedGeneration() const { return proposedGeneration_; }
            void set_ProposedGeneration(Reliability::GenerationNumber const& value) { proposedGeneration_ = value; }

            __declspec (property(get=get_ReceiveGeneration, put=set_ReceiveGeneration)) Reliability::GenerationNumber const& ReceiveGeneration;
            Reliability::GenerationNumber const& get_ReceiveGeneration() const { return receiveGeneration_; }
            void set_ReceiveGeneration(Reliability::GenerationNumber const& value) { receiveGeneration_ = value; }

            __declspec (property(get=get_SendGeneration, put=set_SendGeneration)) Reliability::GenerationNumber const& SendGeneration;
            Reliability::GenerationNumber const& get_SendGeneration() const { return sendGeneration_; }
            void set_SendGeneration(Reliability::GenerationNumber const& value) { sendGeneration_ = value; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
            {
                w.Write("{0}/{1}/{2}", proposedGeneration_, receiveGeneration_, sendGeneration_);
            }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
            {   
                std::string format = "{0}/{1}/{2}";
                size_t index = 0;

                traceEvent.AddEventField<Reliability::GenerationNumber>(format, name + ".proposed", index);
                traceEvent.AddEventField<Reliability::GenerationNumber>(format, name + ".received", index);
                traceEvent.AddEventField<Reliability::GenerationNumber>(format, name + ".send", index);

                return format;
            }

            void FillEventData(Common::TraceEventContext & context) const
            {
                context.Write<Reliability::GenerationNumber>(proposedGeneration_);
                context.Write<Reliability::GenerationNumber>(receiveGeneration_);
                context.Write<Reliability::GenerationNumber>(sendGeneration_);
            }


        private:
            Reliability::GenerationNumber proposedGeneration_;
            Reliability::GenerationNumber receiveGeneration_;
            Reliability::GenerationNumber sendGeneration_;
        };
    }
}
