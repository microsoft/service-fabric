// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureResourceACLsRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureResourceACLsRequest();
        ConfigureResourceACLsRequest(
            std::vector<CertificateAccessDescription> const & certificateAccessDescription,
            std::vector<std::wstring> const & sids,
            DWORD accessMask,
            std::vector<std::wstring> const & applicationFolders,
            ULONG applicationCounter,
            std::wstring const & nodeId,
            int64 timeoutTicks_);

        __declspec(property(get=get_SIDs)) std::vector<std::wstring> const & SIDs;
        std::vector<std::wstring> const & get_SIDs() const { return sids_; }

        __declspec(property(get=get_CertificateAccessDescriptions)) std::vector<CertificateAccessDescription> const & CertificateAccessDescriptions;
        std::vector<CertificateAccessDescription> const & get_CertificateAccessDescriptions() const { return certificateAccessDescriptions_; }

        __declspec(property(get=get_ApplicationFolders)) std::vector<wstring> const & ApplicationFolders;
        std::vector<wstring> const & get_ApplicationFolders() const { return applicationFolders_; }

        __declspec(property(get=get_AccessMask)) DWORD AccessMask;
        DWORD get_AccessMask() const { return accessMask_; }

        __declspec(property(get=get_TimeoutTicks)) int64 TimeoutTicks;
        int64 get_TimeoutTicks() const { return accessMask_; }

        __declspec(property(get = get_ApplicationCounter)) ULONG ApplicationCounter;
        ULONG get_ApplicationCounter() const { return applicationCounter_; }

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(sids_, accessMask_, certificateAccessDescriptions_, applicationFolders_, timeoutTicks_, nodeId_, applicationCounter_);

    private:
        std::vector<CertificateAccessDescription> certificateAccessDescriptions_;
        std::vector<std::wstring> applicationFolders_;
        DWORD accessMask_;
        std::vector<std::wstring> sids_;
        int64 timeoutTicks_;
        ULONG applicationCounter_;
        std::wstring nodeId_;
    };
}
