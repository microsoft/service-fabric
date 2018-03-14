// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class FabricCodeVersion : public Serialization::FabricSerializable
    {
    public:
        static Global<FabricCodeVersion> Invalid;
        static Global<FabricCodeVersion> Default;        
        static Global<FabricCodeVersion> Baseline;
        static Global<FabricCodeVersion> MinCompatible;

        FabricCodeVersion();
        FabricCodeVersion(uint majorVersion, uint minorVersion, uint buildVersion, uint hotfixVersion);
        FabricCodeVersion(FabricCodeVersion const & other);
        FabricCodeVersion(FabricCodeVersion && other);
        virtual ~FabricCodeVersion();

        __declspec(property(get=get_MajorVersion)) uint MajorVersion;
        uint get_MajorVersion() const { return majorVersion_; }

        __declspec(property(get=get_MinorVersion)) uint MinorVersion;
        uint get_MinorVersion() const { return minorVersion_; }

        __declspec(property(get=get_BuildVersion)) uint BuildVersion;
        uint get_BuildVersion() const { return buildVersion_; }

        __declspec(property(get=get_HotfixVersion)) uint HotfixVersion;
        uint get_HotfixVersion() const { return hotfixVersion_; }

        __declspec(property(get=get_IsValid)) bool IsValid;
        bool get_IsValid() const;

        bool IsBaseline() const;

        FabricCodeVersion const & operator = (FabricCodeVersion const & other);
        FabricCodeVersion const & operator = (FabricCodeVersion && other);

        int compare(FabricCodeVersion const & other) const;
        bool operator < (FabricCodeVersion const & other) const;
        bool operator == (FabricCodeVersion const & other) const;
        bool operator != (FabricCodeVersion const & other) const;

        void WriteTo(__in TextWriter&, FormatOptions const &) const;
        
        std::wstring ToString() const;
        static ErrorCode FromString(std::wstring const & fabricCodeVersionString, __out FabricCodeVersion & fabricCodeVersion);
        static bool TryParse(std::wstring const &, __out FabricCodeVersion &);
        
        FABRIC_FIELDS_04(majorVersion_, minorVersion_, buildVersion_, hotfixVersion_);

        static std::string AddField(TraceEvent & traceEvent, std::string const & name);
        void FillEventData(TraceEventContext & context) const;

    private:
        uint majorVersion_;
        uint minorVersion_;
        uint buildVersion_;
        uint hotfixVersion_;

        static GlobalWString Delimiter;
    };
}
DEFINE_USER_ARRAY_UTILITY(Common::FabricCodeVersion);
