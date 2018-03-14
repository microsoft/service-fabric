// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IImageStoreClient
    {
    public:
        virtual ~IImageStoreClient() {};

        virtual Common::ErrorCode Upload(
            std::wstring const & imageStoreConnectionString,
            std::wstring const & sourceFullpath,
            std::wstring const & destinationRelativePath,
            bool const shouldOverwrite) = 0;

        virtual Common::ErrorCode Delete(
            std::wstring const & imageStoreConnectionString,
            std::wstring const & relativePath) = 0;
    };
}
