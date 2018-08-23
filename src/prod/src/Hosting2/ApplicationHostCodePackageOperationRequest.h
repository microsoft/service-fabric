// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace ApplicationHostCodePackageOperationType
    {
        enum Enum : ULONG
        {
            Invalid = 0,
            Activate = 1,
            Deactivate = 2,
            Abort = 3,
            ActivateAll = 4,
            DeactivateAll = 5,
            AbortAll = 6,
            FirstValidEnum = Invalid,
            LastValidEnum = AbortAll
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(ApplicationHostCodePackageOperationType)
    };

    class ApplicationHostCodePackageOperationRequest : public Serialization::FabricSerializable
    {
    public:
        ApplicationHostCodePackageOperationRequest();

        ApplicationHostCodePackageOperationRequest(
            ApplicationHostCodePackageOperationType::Enum operationType,
            ApplicationHostContext const & appHostContext,
            CodePackageContext const & codePackageContext,
            std::vector<std::wstring> const & codePackageNames,
            Common::EnvironmentMap const & environment,
            int64 timeoutTicks);

        __declspec(property(get = get_OperationType)) ApplicationHostCodePackageOperationType::Enum OperationType;
        inline  ApplicationHostCodePackageOperationType::Enum get_OperationType() const { return operationType_; }

        __declspec(property(get = get_IsActivateOperation)) bool IsActivateOperation;
        inline  bool get_IsActivateOperation() const
        { 
            return (
                operationType_ == ApplicationHostCodePackageOperationType::Activate ||
                operationType_ == ApplicationHostCodePackageOperationType::ActivateAll);
        }

        __declspec(property(get = get_IsDeactivateOperation)) bool IsDeactivateOperation;
        inline  bool get_IsDeactivateOperation() const
        {
            return (
                operationType_ == ApplicationHostCodePackageOperationType::Deactivate ||
                operationType_ == ApplicationHostCodePackageOperationType::DeactivateAll);
        }

        __declspec(property(get = get_IsAbortOperation)) bool IsAbortOperation;
        inline  bool get_IsAbortOperation() const
        {
            return (
                operationType_ == ApplicationHostCodePackageOperationType::Abort ||
                operationType_ == ApplicationHostCodePackageOperationType::AbortAll);
        }

        __declspec(property(get = get_IsAllCodePackageOperation)) bool IsAllCodePackageOperation;
        inline  bool get_IsAllCodePackageOperation() const
        {
            return(
                operationType_ == ApplicationHostCodePackageOperationType::ActivateAll ||
                operationType_ == ApplicationHostCodePackageOperationType::DeactivateAll ||
                operationType_ == ApplicationHostCodePackageOperationType::AbortAll);
        }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return timeoutTicks_; }

        __declspec(property(get = get_HostContext)) ApplicationHostContext const & HostContext;
        inline  ApplicationHostContext const & get_HostContext() const { return appHostContext_; }

        __declspec(property(get = get_CodeContext)) CodePackageContext const & CodeContext;
        inline  CodePackageContext const & get_CodeContext() const { return codePackageContext_; }

        __declspec(property(get = get_CodePackageNames)) std::vector<std::wstring> const & CodePackageNames;
        inline  std::vector<std::wstring> const & get_CodePackageNames() const { return codePackageNames_; }

        __declspec(property(get = get_EnvironmentBlock)) std::vector<wchar_t> const & EnvironmentBlock;
        inline  std::vector<wchar_t> const & get_EnvironmentBlock() const { return environmentBlock_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(operationType_, appHostContext_, codePackageContext_, codePackageNames_, environmentBlock_, timeoutTicks_);

    private:
        ApplicationHostCodePackageOperationType::Enum operationType_;
        ApplicationHostContext appHostContext_;
        CodePackageContext codePackageContext_;
        std::vector<std::wstring> codePackageNames_;
        std::vector<wchar_t> environmentBlock_;
        int64 timeoutTicks_;
    };
}
