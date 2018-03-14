// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{         
    class ImpersonationContext;
    typedef std::unique_ptr<ImpersonationContext> ImpersonationContextUPtr;

    class ImpersonationContext
    {
        DENY_COPY(ImpersonationContext)

    public:
        ImpersonationContext(TokenHandleSPtr const & accessToken);
        ~ImpersonationContext();

        ErrorCode ImpersonateLoggedOnUser();
        ErrorCode RevertToSelf();

    private:
        TokenHandleSPtr tokenHandle_;
        bool impersonating_;        
        RwLock lock_;
    };
}
