// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ProfileHandle;
    typedef std::unique_ptr<ProfileHandle> ProfileHandleUPtr;
    typedef std::shared_ptr<ProfileHandle> ProfileHandleSPtr;

    class ProfileHandle : public HandleBase<HANDLE>
    {
        DENY_COPY(ProfileHandle)

    public:
        explicit ProfileHandle(TokenHandleSPtr const & tokenHandle, HANDLE hValue);
        virtual ~ProfileHandle();
        static ProfileHandleSPtr CreateSPtr(TokenHandleSPtr const & tokenHandle, HANDLE hValue);
        static ProfileHandleUPtr CreateUPtr(TokenHandleSPtr const & tokenHandle, HANDLE hValue);
        static void Cleanup(TokenHandleSPtr const & tokenHandle, HANDLE hValue);

    private:
        TokenHandleSPtr const tokenHandle_;
    };
}
