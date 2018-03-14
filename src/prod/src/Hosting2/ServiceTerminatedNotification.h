// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ServiceTerminatedNotification : public Serialization::FabricSerializable
    {
    public:
        ServiceTerminatedNotification();
        ServiceTerminatedNotification(
            std::wstring const & parentId,
            std::vector<std::wstring> const & appServiceIds,
            DWORD exitCode);


        __declspec(property(get=get_ParentId)) std::wstring const & ParentId;
        std::wstring const & get_ParentId() const { return parentId_; }

        __declspec(property(get=get_AppServiceId)) std::vector<std::wstring> const & ApplicationServiceIds;
        std::vector<std::wstring> const & get_AppServiceId() const { return appServiceIds_; }

        __declspec(property(get=get_ExitCode)) DWORD ExitCode;
        DWORD get_ExitCode() const { return exitCode_; }


        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(parentId_, appServiceIds_, exitCode_);

    private:
        std::wstring parentId_;
        std::vector<std::wstring> appServiceIds_;
        DWORD exitCode_;
    };
}
