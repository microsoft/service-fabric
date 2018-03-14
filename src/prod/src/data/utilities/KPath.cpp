// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;

#if !defined(PLATFORM_UNIX)
const KStringView KPath::GlobalDosDevicesNamespace(L"\\??\\");
#endif

KString::SPtr KPath::Combine(
    __in KStringView const & path1,
    __in KStringView const & path2,
    __in KAllocator & allocator)
{
    ASSERT_IF(path1.IsNull(), "Path::Combine: Path1 must not be null");
    ASSERT_IF(path2.IsNull(), "Path::Combine: Path2 must not be null");

    KString::SPtr newPath;
    NTSTATUS status = KString::Create(newPath, allocator, path1);
    THROW_ON_FAILURE(status);

    if (path2.IsEmpty() == TRUE)
    {
        return newPath;
    }

    if (path1.IsEmpty() == FALSE && path1[path1.Length() - 1] != KVolumeNamespace::PathSeparatorChar)
    {
        BOOLEAN boolStatus = newPath->Concat(KVolumeNamespace::PathSeparator);
        ASSERT_IFNOT(boolStatus == TRUE, "Path::Combine concat failed.");
    }

    BOOLEAN boolStatus = newPath->Concat(path2);
    ASSERT_IFNOT(boolStatus == TRUE, "Path::Combine concat failed.");
    
    return Ktl::Move(newPath);
}

void KPath::CombineInPlace(
    __inout KString & path1,
    __in KStringView const & path2)
{
    ASSERT_IF(path1.IsNull(), "Path::Combine: Path1 must not be null");
    ASSERT_IF(path2.IsNull(), "Path::Combine: Path2 must not be null");

    if (path2.IsEmpty() == TRUE)
    {
        return;
    }

    if (path1.IsEmpty() == FALSE && path1[path1.Length() - 1] != KVolumeNamespace::PathSeparatorChar)
    {
        BOOLEAN boolStatus = path1.Concat(KVolumeNamespace::PathSeparator);
        ASSERT_IFNOT(boolStatus == TRUE, "Path::Combine concat failed.");
    }

    BOOLEAN boolStatus = path1.Concat(path2);
    ASSERT_IFNOT(boolStatus == TRUE, "Path::Combine concat failed.");

    return;
}

KString::SPtr KPath::CreatePath(
    __in KStringView const & path,
    __in KAllocator & allocator)
{
    ASSERT_IF(path.IsNull() || path.IsEmpty(), "Path must not be null");

    KString::SPtr result = nullptr;
#if !defined(PLATFORM_UNIX)
    bool startsWithGCN = StartsWithGlobalDosDevicesNamespace(path);
    if (startsWithGCN == false)
    {
        result = KPath::Combine(GlobalDosDevicesNamespace, path, allocator);
    }
    else 
    {
        NTSTATUS status = KString::Create(result, allocator, path);
        THROW_ON_FAILURE(status);
    }
#else
    NTSTATUS status = KString::Create(result, allocator, path);
    THROW_ON_FAILURE(status);
#endif

    return result;
}

#if !defined(PLATFORM_UNIX)
bool KPath::StartsWithGlobalDosDevicesNamespace(
    __in KStringView const & path) 
{
    ASSERT_IF(path.IsNull(), "Path must not be null");

    if (path.Length() < 4)
    {
        return false;
    }

    return path[0] == KVolumeNamespace::PathSeparatorChar &&  path[1] == L'?'  &&  path[2] == L'?'  &&  path[3] == KVolumeNamespace::PathSeparatorChar;
}

NTSTATUS KPath::RemoveGlobalDosDevicesNamespaceIfExist(
    __in KString const & directoryPath,
    __in KAllocator & allocator,
    __out KString::CSPtr & result) noexcept
{
    ULONG pos = 0;
    BOOLEAN exist = directoryPath.Search(GlobalDosDevicesNamespace, pos);
    if (pos == 0 && exist == TRUE)
    {
        ULONG prefixLength = GlobalDosDevicesNamespace.Length();
        ULONG dirLength = directoryPath.Length();

        KString::SPtr newPath = nullptr;
        NTSTATUS status = KString::Create(
            newPath,
            allocator,
            directoryPath.SubString(prefixLength, dirLength - prefixLength));

        result = NT_SUCCESS(status) ? Ktl::Move(newPath.RawPtr()) : nullptr;
        return status;
    }

    result = &directoryPath;
    return STATUS_SUCCESS;
}
#endif
