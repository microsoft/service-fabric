// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // The generation number is used for rebuild GFUM to prevent messages in 
    // previous generation interfere with the new generation.
    class GenerationNumber : public Serialization::FabricSerializable
    {
    public:
        GenerationNumber();

        GenerationNumber(int64 generation, Federation::NodeId const & owner);

        GenerationNumber(GenerationNumber const & oldGeneration, Federation::NodeId const & owner);

        __declspec(property(get=get_Value)) int64 Value;
        int64 get_Value() const { return generation_; }

        bool operator==(GenerationNumber const & other) const;
        bool operator!=(GenerationNumber const & other) const;
        bool operator<(GenerationNumber const & other) const;
        bool operator>(GenerationNumber const & other) const;
        bool operator<=(GenerationNumber const & other) const;
        bool operator>=(GenerationNumber const & other) const;

        // Represents the lowest generation number.
        const static GenerationNumber Zero;

        std::wstring ToString() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {   
            std::string format = "{0}:{1}";
            size_t index = 0;

            traceEvent.AddEventField<int64>(format, name + ".generation", index);
            traceEvent.AddEventField<Federation::NodeId>(format, name + ".owner", index);
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<int64>(generation_);
            context.Write<Federation::NodeId>(owner_);
        }



        FABRIC_FIELDS_02(generation_, owner_);

    private:
        int64 generation_;
        Federation::NodeId owner_;
    };
}
