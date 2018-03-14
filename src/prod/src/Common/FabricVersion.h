// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FabricVersion : public Serialization::FabricSerializable
    {
    public:
        static GlobalWString Delimiter;

        // following are defined in FabricVersionConstants.cpp
        static Global<FabricVersion> Baseline;
        static Global<FabricVersion> Default;
        static Global<FabricVersion> Invalid;

        FabricVersion();
        FabricVersion(FabricCodeVersion const & codeVersion, FabricConfigVersion const & configVersion);
        FabricVersion(FabricVersion const & other);
        FabricVersion(FabricVersion && other);
        virtual ~FabricVersion();

        __declspec(property(get=get_CodeVersion)) FabricCodeVersion const & CodeVersion;
        FabricCodeVersion const & get_CodeVersion() const { return codeVersion_; }

        __declspec(property(get=get_ConfigVersion)) FabricConfigVersion const & ConfigVersion;
        FabricConfigVersion const & get_ConfigVersion() const { return configVersion_; }

        __declspec(property(get=get_IsValid)) bool IsValid;
        bool get_IsValid() const { return (codeVersion_.IsValid && configVersion_.IsValid); }

        FabricVersion const & operator = (FabricVersion const & other);
        FabricVersion const & operator = (FabricVersion && other);

        bool operator == (FabricVersion const & other) const;
        bool operator != (FabricVersion const & other) const;

        int compare(FabricVersion const & other) const;
        bool operator < (FabricVersion const & other) const;

        void WriteTo(TextWriter&, FormatOptions const &) const;

        std::wstring ToString() const;
        static ErrorCode FromString(std::wstring const & fabricVersionString, __out FabricVersion & fabricVersion);
        static bool TryParse(std::wstring const & input, __out FabricVersion & fabricVersion);

        FABRIC_FIELDS_02(codeVersion_, configVersion_);

        static std::string AddField(TraceEvent & traceEvent, std::string const & name);
        void FillEventData(TraceEventContext & context) const;

        static FabricVersion const & GetBaselineVersion();

    private:
        FabricCodeVersion codeVersion_;
        FabricConfigVersion configVersion_;
    };
}
