// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Header used for fabric node to dispatch its messages.
    struct GenerationHeader : public Transport::MessageHeader<Transport::MessageHeaderId::Generation>, public Serialization::FabricSerializable
    {
    public:
        GenerationHeader()
        {
        }

        GenerationHeader(GenerationNumber const & generation, bool isForFMReplica)
             :  generation_(generation),
                isForFMReplica_(isForFMReplica)
        {
        }

        __declspec (property(get=get_Generation)) Reliability::GenerationNumber const& Generation;
        Reliability::GenerationNumber const& get_Generation() const { return generation_; }

        __declspec (property(get=get_IsForFMReplica)) bool IsForFMReplica;
        bool get_IsForFMReplica() const { return isForFMReplica_; }

        FABRIC_FIELDS_02(generation_, isForFMReplica_);

        void WriteTo (Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0}:{1}", generation_, isForFMReplica_);
        }

        static GenerationHeader ReadFromMessage(Transport::Message & message)
        {
            GenerationHeader header;
            bool found = message.Headers.TryReadFirst(header);
            ASSERT_IFNOT(found, "Generation header not found in {0} message", message.Action);
            return header;
        }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {   
            std::string format = "{0}:{1}";
            size_t index = 0;

            traceEvent.AddEventField<Reliability::GenerationNumber>(format, name + ".generation", index);
            traceEvent.AddEventField<bool>(format, name + ".isForFMReplica", index);            

            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<Reliability::GenerationNumber>(generation_);
            context.Write<bool>(isForFMReplica_);            
        }

    private:
        GenerationNumber generation_;
        bool isForFMReplica_;
    };
}
