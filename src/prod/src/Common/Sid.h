// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class Sid;
    typedef std::shared_ptr<Sid> SidSPtr;
    typedef std::unique_ptr<Sid> SidUPtr;

    class Sid
    {
        DENY_COPY(Sid)

    protected:
        Sid(PSID const pSid);

    public:
        virtual ~Sid();

        __declspec(property(get=get_PSID)) PSID const PSid;
        PSID const get_PSID() const { return pSid_; }

        SID_IDENTIFIER_AUTHORITY GetAuthority() const;
        UCHAR GetSubAuthorityCount() const;
        DWORD GetSubAuthority(DWORD subAuthority) const;
        Common::ErrorCode GetAllowDacl(__out std::wstring & allowDacl) const;
        Common::ErrorCode ToString(__out std::wstring & stringSid) const;

        bool IsLocalSystem() const;
        bool IsNetworkService() const;
        bool IsAnonymous() const;

        static bool IsValid(PSID pSid);
        static Common::ErrorCode LookupAccount(PSID pSid, __out std::wstring & domainName, __out std::wstring & userName);
        static Common::ErrorCode ToString(PSID pSid, __out std::wstring & stringSid);

    private:
        PSID const pSid_;
    };

    class BufferedSid : public Sid
    {
        DENY_COPY(BufferedSid)

    public:
        BufferedSid(Common::ByteBuffer && buffer);
        virtual ~BufferedSid();

        static Common::ErrorCode CreateUPtr(__in PSID pSid, __out SidUPtr & sid);
        static Common::ErrorCode CreateSPtr(__in PSID pSid, __out SidSPtr & sid);

        static Common::ErrorCode CreateUPtrFromStringSid(std::wstring const & stringSid, __out SidUPtr & sid);
        static Common::ErrorCode CreateSPtrFromStringSid(std::wstring const & stringSid, __out SidSPtr & sid);

        static Common::ErrorCode CreateUPtr(std::wstring const & accountName, __out SidUPtr & sid);
        static Common::ErrorCode CreateSPtr(std::wstring const & accountName, __out SidSPtr & sid);

#if defined(PLATFORM_UNIX)
        static Common::ErrorCode CreateGroupSPtr(std::wstring const & accountName, __out SidSPtr & sid);
#endif

        static Common::ErrorCode CreateUPtr(WELL_KNOWN_SID_TYPE wellKnownSid, __out SidUPtr & sid);
        static Common::ErrorCode CreateSPtr(WELL_KNOWN_SID_TYPE wellKnownSid, __out SidSPtr & sid);
        
        static Common::ErrorCode CreateUPtr(SID_IDENTIFIER_AUTHORITY const & identifierAuthority, std::vector<DWORD> const & subAuthority, __out SidUPtr & sid);
        static Common::ErrorCode CreateSPtr(SID_IDENTIFIER_AUTHORITY const & identifierAuthority, std::vector<DWORD> const & subAuthority, __out SidSPtr & sid);

        static Common::ErrorCode GetCurrentUserSid(__out SidUPtr & sid);

        static Common::ErrorCode Create(std::wstring const & accountName, __inout Common::ByteBuffer & buffer);

    private:
        static Common::ErrorCode Create(__in PSID pSid, __inout Common::ByteBuffer & buffer);
        static Common::ErrorCode Create(WELL_KNOWN_SID_TYPE wellKnownSid,  __inout Common::ByteBuffer & buffer);
        static Common::ErrorCode Create(SID_IDENTIFIER_AUTHORITY const & identifierAuthority, std::vector<DWORD> const & subAuthority,  __inout Common::ByteBuffer & buffer);

        static PSID GetPSID(Common::ByteBuffer const & buffer);

    private:
        Common::ByteBuffer const buffer_;
    };
}
