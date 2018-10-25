// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class StartRegisterApplicationHostRequest : public Serialization::FabricSerializable
    {
    public:
        StartRegisterApplicationHostRequest();
        StartRegisterApplicationHostRequest(
            std::wstring const & applicationHostId,
            ApplicationHostType::Enum applicationHostType,
            DWORD applicationHostProcessId,
            bool isContainerHost,
            bool isCodePackageActivatorHost,
            Common::TimeSpan timeout);

        __declspec(property(get = get_hostId)) std::wstring const & Id;
        std::wstring const & get_hostId() const { return id_; }

        __declspec(property(get = get_hostType)) ApplicationHostType::Enum Type;
        ApplicationHostType::Enum get_hostType() const { return type_; }

        __declspec(property(get = get_ProcessId)) DWORD ProcessId;
        DWORD get_ProcessId() const { return processId_; }

        __declspec(property(get = get_IsContainerHost)) bool IsContainerHost;
        bool get_IsContainerHost() const { return isContainerHost_; }

        __declspec(property(get = get_IsCodePackageActivatorHost)) bool IsCodePackageActivatorHost;
        bool get_IsCodePackageActivatorHost() const { return isCodePackageActivatorHost_; }

        __declspec(property(get = get_Timeout)) Common::TimeSpan Timeout;
        Common::TimeSpan get_Timeout() const { return timeout_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_06(id_, type_, processId_, timeout_, isContainerHost_, isCodePackageActivatorHost_);

    private:
        std::wstring id_;
        ApplicationHostType::Enum type_;
        DWORD processId_;
        bool isContainerHost_;
        bool isCodePackageActivatorHost_;
        Common::TimeSpan timeout_;
    };
}
