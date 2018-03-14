// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FabricVersionInstance : public Serialization::FabricSerializable
    {
    public:
        static Global<FabricVersionInstance> Baseline;
        static Global<FabricVersionInstance> Default;
        static Global<FabricVersionInstance> Invalid;

        FabricVersionInstance();
        FabricVersionInstance(FabricVersion const & version, uint64 instanceId);
        FabricVersionInstance(FabricVersionInstance const & other);
        FabricVersionInstance(FabricVersionInstance && other);
        virtual ~FabricVersionInstance();

        __declspec(property(get=get_Version)) FabricVersion const & Version;
        FabricVersion const & get_Version() const { return version_; }

        __declspec(property(get=get_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }

        FabricVersionInstance const & operator = (FabricVersionInstance const & other);
        FabricVersionInstance const & operator = (FabricVersionInstance && other);

        bool operator == (FabricVersionInstance const & other) const;
        bool operator != (FabricVersionInstance const & other) const;

        int compare(FabricVersionInstance const & other) const;
        bool operator < (FabricVersionInstance const & other) const;

        void WriteTo(TextWriter&, FormatOptions const &) const;

        std::wstring ToString() const;
        static ErrorCode FromString(std::wstring const & fabricVersionInstanceString, __out FabricVersionInstance & fabricVersionInstance);

        static std::string AddField(TraceEvent & traceEvent, std::string const & name);
        void FillEventData(TraceEventContext & context) const;        

        FABRIC_FIELDS_02(version_, instanceId_);

    public:
        static GlobalWString Delimiter;

    private:
        FabricVersion version_;
        uint64 instanceId_;            
        
    };
}
