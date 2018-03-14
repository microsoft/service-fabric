// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ConfigureSMBShareRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureSMBShareRequest();
        ConfigureSMBShareRequest(
            std::vector<std::wstring> const & sids,
            DWORD accessMask,
            std::wstring const & localFullPath,
            std::wstring const & shareName,
            int64 timeoutTicks);

        __declspec(property(get = get_Sids)) std::vector<std::wstring> const & Sids;
        std::vector<std::wstring> const & get_Sids() const { return sids_; }

        __declspec(property(get = get_AccessMask)) DWORD AccessMask;
        DWORD get_AccessMask() const { return accessMask_; }

        __declspec(property(get = get_LocalFullPath)) std::wstring const & LocalFullPath;
        std::wstring const & get_LocalFullPath() const { return localFullPath_; }

        __declspec(property(get = get_ShareName)) std::wstring const & ShareName;
        std::wstring const & get_ShareName() const { return shareName_; }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return ticks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(sids_, accessMask_, localFullPath_, shareName_, ticks_);

    private:
        std::vector<std::wstring> sids_;
        DWORD accessMask_;
        std::wstring localFullPath_;
        std::wstring shareName_;
        int64 ticks_;
    };
}
