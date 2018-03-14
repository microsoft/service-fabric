// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        interface IStoreCopyProvider
        {
            K_SHARED_INTERFACE(IStoreCopyProvider)
        public:
            __declspec(property(get = get_WorkingDirectory)) KString::CSPtr WorkingDirectoryCSPtr;
            virtual KString::CSPtr get_WorkingDirectory() const = 0;

            virtual ktl::Awaitable<KSharedPtr<MetadataTable>> GetMetadataTableAsync() = 0;
        };
    }
}
