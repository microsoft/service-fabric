// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class Dacl;
    typedef std::shared_ptr<Dacl> DaclSPtr;

    class Dacl : public std::enable_shared_from_this<Dacl>
    {
        DENY_COPY(Dacl)

    public:
        class ACEInfo
        {
        public:
            ACEInfo(BYTE aceType, BYTE aceFlags, ACCESS_MASK accessMask, PSID pSid);
            ACEInfo(ACEInfo const & other);
            ACEInfo(ACEInfo && other);
            ACEInfo const & operator = (ACEInfo const & other);
            ACEInfo const & operator = (ACEInfo && other);
            DWORD GetLength() const;
            bool IsValid() const;

            __declspec(property(get=get_AceType)) BYTE const & AceType;
            BYTE const & get_AceType() const { return aceType_; }

            __declspec(property(get=get_AceFlags)) BYTE const & AceFlags;
            BYTE const & get_AceFlags() const { return aceFlags_; }

            __declspec(property(get=get_AccessMask)) ACCESS_MASK const & AccessMask;
            ACCESS_MASK const & get_AccessMask() const { return accessMask_; }

            __declspec(property(get=get_PSID)) PSID const & PSid;
            PSID const & get_PSID() const { return pSid_; }

        private:
            BYTE aceType_;
            BYTE aceFlags_;
            ACCESS_MASK accessMask_;
            PSID pSid_;
        };

    protected:
        Dacl(PACL pAcl);

    public:
        virtual ~Dacl();

        __declspec(property(get=get_PACL)) PACL const & PAcl;
        PACL const & get_PACL() const { return pAcl_; }

        static bool IsValid(PACL pAcl);

    private:
        PACL const pAcl_;
    };

    class BufferedDacl : public Dacl
    {
        DENY_COPY(BufferedDacl)

    public:
        BufferedDacl(Common::ByteBuffer && buffer);
        virtual ~BufferedDacl();
        Common::ErrorCode ContainsACE(Common::SidSPtr const & sid, DWORD accessMask, bool & containsACE);

    private:
        typedef std::function<Common::ErrorCode(void * pAce, __out bool & isValid)> AceValidationCallback;

    public:
        static Common::ErrorCode CreateSPtr(std::vector<Dacl::ACEInfo> entries, __out DaclSPtr & dacl);
        static Common::ErrorCode CreateSPtr(PACL const existingAcl, __out DaclSPtr & dacl);
        static Common::ErrorCode CreateSPtr(PACL const existingAcl, std::vector<Dacl::ACEInfo> additionalEntries, __out DaclSPtr & dacl);
        static Common::ErrorCode CreateSPtr(PACL const existingAcl, std::vector<Common::SidSPtr> const & removeACLsOn, __out DaclSPtr & dacl);
        static Common::ErrorCode CreateSPtr(PACL const existingAcl, bool removeInvalid, __out DaclSPtr & dacl);
        static Common::ErrorCode CreateSPtr(
            PACL const existingAcl,
            std::vector<Dacl::ACEInfo> const & additionalEntries,
            bool removeInvalid,
            __out DaclSPtr & dacl);

        static PACL GetPACL(Common::ByteBuffer const & buffer);

    private:
        static Common::ErrorCode CreateSPtr_ValidateExistingAce(
            PACL const existingAcl, 
            std::vector<Dacl::ACEInfo> const & additionalEntries,
            AceValidationCallback const & validationCallback,
            __out DaclSPtr & dacl);
        static Common::ErrorCode ValidateSid(void * pAce, __out bool & isValid);
        static Common::ErrorCode ContainsSid(void * pAce, std::vector<std::wstring> const & targetSids, __out bool & isContains);

        Common::ByteBuffer const buffer_;
    };
}
