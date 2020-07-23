// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SecurityPrincipalHelper : protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:

        static Common::ErrorCode AddMemberToLocalGroup(std::wstring const & parentGroupAccountName, std::wstring const & memberToAddAccountName);

        static Common::ErrorCode CreateUserAccount(std::wstring const & accountName, std::wstring const & password, std::wstring const & comment);
        static Common::ErrorCode CreateGroupAccount(std::wstring const & groupName, std::wstring const & comment);

        static Common::ErrorCode DeleteUserProfile(std::wstring const & accountName, Common::SidSPtr const& sid = Common::SidSPtr());
        static Common::ErrorCode DeleteUserAccount(std::wstring const & accountName);
        static Common::ErrorCode DeleteUserAccountIgnoreDeleteProfileError(std::wstring const & accountName, Common::SidSPtr const& sid = Common::SidSPtr());

        static Common::ErrorCode DeleteGroupAccount(std::wstring const & groupName);

        static Common::ErrorCode GetMembership(std::wstring const & accountName, __out std::vector<std::wstring> & memberOf);
        static Common::ErrorCode GetMembers(std::wstring const & accountName, __out std::vector<std::wstring> & members);

        static Common::ErrorCode GetGroupComment(std::wstring const & accountName, __out std::wstring & comment);
        static Common::ErrorCode GetUserComment(std::wstring const & accountName, __out std::wstring & comment);

        static Common::ErrorCode UpdateGroupComment(std::wstring const & accountName, std::wstring const& comment);
        static Common::ErrorCode UpdateUserComment(std::wstring const & accountName, std::wstring const& comment);

        static Common::ErrorCode AcquireMutex(std::wstring const& mutexName, __out Common::MutexHandleUPtr & mutex);
    };
}
