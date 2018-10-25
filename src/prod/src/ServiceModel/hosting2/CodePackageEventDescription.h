// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    namespace CodePackageEventType
    {
        enum Enum : ULONG
        {
            Invalid = 0,
            StartFailed = 1,
            Started = 2,
            Ready = 3,
            Health = 4,
            Stopped = 5,
            Terminated = 6,
            FirstValidEnum = Invalid,
            LastValidEnum = Terminated
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        ::FABRIC_CODE_PACKAGE_EVENT_TYPE ToPublicApi(__in Enum);
        Common::ErrorCode FromPublicApi(__in::FABRIC_CODE_PACKAGE_EVENT_TYPE, __out Enum &);

        DECLARE_ENUM_STRUCTURED_TRACE(CodePackageEventType)
    };

    class CodePackageEventProperties
    {
    public:
        static std::wstring const ExitCode;
        static std::wstring const ErrorMessage;
        static std::wstring const FailureCount;
        static std::wstring const Version;
    };

    class CodePackageEventDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(CodePackageEventDescription)
        DEFAULT_COPY_CONSTRUCTOR(CodePackageEventDescription)

        DEFAULT_MOVE_ASSIGNMENT(CodePackageEventDescription)
        DEFAULT_MOVE_CONSTRUCTOR(CodePackageEventDescription)

    public:
        CodePackageEventDescription();
        CodePackageEventDescription(
            std::wstring const & codePackageName,
            bool isSetupEntryPoint,
            bool isContainerHost,
            CodePackageEventType::Enum eventType,
            Common::DateTime timeStamp,
            int64 squenceNumber,
            std::map<std::wstring, std::wstring> const & properties);

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        inline std::wstring const & get_CodePackageName() const { return codePackageName_; }

        __declspec(property(get = get_IsSetupEntryPoint)) bool IsSetupEntryPoint;
        inline bool get_IsSetupEntryPoint() const { return isSetupEntryPoint_; }

        __declspec(property(get = get_IsContainerHost)) bool IsContainerHost;
        inline bool get_IsContainerHost() const { return isContainerHost_; }

        __declspec(property(get = get_EventType)) CodePackageEventType::Enum const & EventType;
        inline CodePackageEventType::Enum const & get_EventType() const { return eventType_; };

        __declspec(property(get = get_TimeStamp)) Common::DateTime TimeStamp;
        Common::DateTime get_TimeStamp() const { return timeStamp_; }

        __declspec(property(get = get_SquenceNumber)) int64 SquenceNumber;
        int64 get_SquenceNumber() const { return squenceNumber_; }

        __declspec(property(get = get_Properties))  std::map<std::wstring, std::wstring> const & Properties;
        inline std::map<std::wstring, std::wstring> const & get_Properties() const { return properties_; };

    public:
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out ::FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION & eventDesc) const;

        Common::ErrorCode FromPublicApi(FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION const & eventDesc);

        FABRIC_FIELDS_06(codePackageName_, isSetupEntryPoint_, isContainerHost_, eventType_, properties_, squenceNumber_);

    private:
        std::wstring codePackageName_;
        bool isSetupEntryPoint_;
        bool isContainerHost_;
        CodePackageEventType::Enum eventType_;
        Common::DateTime timeStamp_;
        int64 squenceNumber_;
        std::map<std::wstring, std::wstring> properties_;
    };
}
