// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class TokenHandle;
    typedef std::unique_ptr<TokenHandle> TokenHandleUPtr;
    typedef std::shared_ptr<TokenHandle> TokenHandleSPtr;

    class TokenHandle : public HandleBase<HANDLE>
    {
        DENY_COPY(TokenHandle)

    public:
        explicit TokenHandle(HANDLE hValue);
        virtual ~TokenHandle();
        static TokenHandleSPtr CreateSPtr(HANDLE hValue);
        static TokenHandleUPtr CreateUPtr(HANDLE hValue);
        static void Cleanup(HANDLE hValue);
    };
}
