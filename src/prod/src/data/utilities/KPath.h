// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class KPath
        {
        public:
            static KString::SPtr Combine(
                __in KStringView const & path1,
                __in KStringView const & path2,
                __in KAllocator & allocator);

            static void CombineInPlace(
                __inout KString & path,
                __in KStringView const & otherPath);

            static KString::SPtr CreatePath(
                __in KStringView const & path,
                __in KAllocator & allocator);

#if !defined(PLATFORM_UNIX)
            static const KStringView GlobalDosDevicesNamespace;

            static bool StartsWithGlobalDosDevicesNamespace(
                __in KStringView const & path);

            static NTSTATUS RemoveGlobalDosDevicesNamespaceIfExist(
                __in KString const & directoryPath,
                __in KAllocator & allocator,
                __out KString::CSPtr & result) noexcept;
#endif
        };
    }
}
