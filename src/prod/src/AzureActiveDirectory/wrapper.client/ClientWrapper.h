// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace AzureActiveDirectory
{
    class ClientWrapper
    {
    public:

#if !defined(PLATFORM_UNIX)
        static Common::ErrorCode GetToken(
            std::shared_ptr<Transport::ClaimsRetrievalMetadata> const & metadata,
            size_t bufferSize,
            bool refreshClaimsToken,
            __out std::wstring & token);
#else
        static Common::ErrorCode GetToken(
            std::shared_ptr<Transport::ClaimsRetrievalMetadata> const &,
            size_t,
            bool,
            __out std::wstring &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }
#endif
    };
}
