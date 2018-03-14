// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class LogCollectionProviderFactory
    {
        DENY_COPY(LogCollectionProviderFactory)

    public:
        static Common::ErrorCode CreateLogCollectionProvider(
            Common::ComponentRoot const & root,
            std::wstring const & nodeId,
            std::wstring const & nodePath,
            __out std::unique_ptr<ILogCollectionProvider> & provider);

    private:
        class DummyLogCollectionProvider;
    };
}
