// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class SecurityDescriptor;
    typedef std::shared_ptr<SecurityDescriptor> SecurityDescriptorSPtr;    

    // SecurityDescriptor is used to set DACL on folder and processes
    class SecurityDescriptor
    {
        DENY_COPY(SecurityDescriptor)

    protected:
        SecurityDescriptor(const PSECURITY_DESCRIPTOR pSecurityDescriptor);

    public:
        virtual ~SecurityDescriptor();

        __declspec(property(get=get_PSECURITY_DESCRIPTOR)) PSECURITY_DESCRIPTOR PSecurityDescriptor;
        PSECURITY_DESCRIPTOR get_PSECURITY_DESCRIPTOR() const { return pSecurityDescriptor_; }

        __declspec(property(get=get_Length)) ULONG Length;
        ULONG get_Length() const;

        bool IsEquals(const PSECURITY_DESCRIPTOR pDescriptor) const;
        static bool IsValid(PSECURITY_DESCRIPTOR pSecurityDescriptor);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        PSECURITY_DESCRIPTOR const pSecurityDescriptor_;
    };

    class BufferedSecurityDescriptor : public SecurityDescriptor
    {
        DENY_COPY(BufferedSecurityDescriptor)

    public:
        BufferedSecurityDescriptor(Common::ByteBuffer && buffer);
        virtual ~BufferedSecurityDescriptor();

        static Common::ErrorCode CreateSPtr(__out SecurityDescriptorSPtr & sd);
        static Common::ErrorCode CreateSPtrFromKernelObject(HANDLE object, __out SecurityDescriptorSPtr & sd);
        static Common::ErrorCode CreateSPtr(std::wstring const & accountName, __out SecurityDescriptorSPtr & sd);        
        static Common::ErrorCode CreateSPtr(PSECURITY_DESCRIPTOR const pSecurityDescriptor, __out SecurityDescriptorSPtr & sd);

        // Create self-relative security descriptor from owner token and allowed SIDs.
        // The DACL will contain allowed ACE for the owner SID and each SID in allowedSIDs.
        static Common::ErrorCode CreateAllowed(
            HANDLE ownerToken,
            bool allowOwner,
            std::vector<Common::SidSPtr> const & allowedSIDs,
            DWORD accessMask,
            _Out_ SecurityDescriptorSPtr & sd);

        static Common::ErrorCode CreateAllowed(            
            std::vector<Common::SidSPtr> const & allowedSIDs,
            DWORD accessMask,
            _Out_ SecurityDescriptorSPtr & sd);

        static Common::ErrorCode ConvertSecurityDescriptorStringToSecurityDescriptor(
            std::wstring const & stringSecurityDescriptor,
             __out SecurityDescriptorSPtr & sd);

        static PSECURITY_DESCRIPTOR GetPSECURITY_DESCRIPTOR(Common::ByteBuffer const & buffer);

    private:
        Common::ByteBuffer const buffer_;
    };
}
