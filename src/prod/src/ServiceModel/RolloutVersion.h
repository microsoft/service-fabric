// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class RolloutVersion : public Serialization::FabricSerializable
    {
    public:
        RolloutVersion();
        RolloutVersion(ULONG major, ULONG minor);
        RolloutVersion(RolloutVersion const & other);
        RolloutVersion(RolloutVersion && other);
        virtual ~RolloutVersion();

        static const RolloutVersion Zero;

        __declspec(property(get=get_Major)) ULONG Major;
        ULONG get_Major() const { return major_; }

        __declspec(property(get=get_Minor)) ULONG Minor;
        ULONG get_Minor() const { return minor_; }

        RolloutVersion NextMajor() const;
        RolloutVersion NextMinor() const;

        RolloutVersion const & operator = (RolloutVersion const & other);
        RolloutVersion const & operator = (RolloutVersion && other);

        bool operator == (RolloutVersion const & other) const;
        bool operator != (RolloutVersion const & other) const;

        int compare(RolloutVersion const & other) const;
        bool operator < (RolloutVersion const & other) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & rolloutVersionString, __out RolloutVersion & rolloutVersion);

        FABRIC_FIELDS_02(major_, minor_);

        const static RolloutVersion Invalid;
                
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {            
            traceEvent.AddField<uint32>(name + ".major");
            traceEvent.AddField<uint32>(name + ".minor");
            return "{0}:{1}";
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.WriteCopy<uint32>(static_cast<uint32>(major_));
            context.WriteCopy<uint32>(static_cast<uint32>(minor_));
        }

    private:
        ULONG major_;
        ULONG minor_;
    };
}
