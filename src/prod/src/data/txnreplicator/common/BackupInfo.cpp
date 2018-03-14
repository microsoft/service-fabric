// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;
using namespace TxnReplicator;

BackupInfo::BackupInfo() noexcept
    : KObject()
    , backupId_()
    , directoryPath_()
    , option_(FABRIC_BACKUP_OPTION_INVALID)
    , version_(BackupVersion::Invalid()) 
{
}

BackupInfo::BackupInfo(
    __in Common::Guid const & backupId,
    __in KString const & directoryPath,
    __in FABRIC_BACKUP_OPTION option,
    __in BackupVersion version,
    __in KAllocator & allocator) noexcept
    : KObject()
    , backupId_(backupId)
    , option_(option)
    , version_(version)
{
#if !defined(PLATFORM_UNIX)
    NTSTATUS status = KPath::RemoveGlobalDosDevicesNamespaceIfExist(directoryPath, allocator, directoryPath_);
    SetConstructorStatus(status);
#else
    directoryPath_ = &directoryPath;
#endif
}

BackupInfo::BackupInfo(
    __in BackupInfo const & other) noexcept
{
    if (this != &other)
    {
        backupId_ = other.backupId_;
        directoryPath_ = other.directoryPath_;
        option_ = other.option_;
        version_ = other.version_;
    }
}

BackupInfo & BackupInfo::operator=(
    __in BackupInfo const & other) noexcept
{
    if (&other == this)
    {
        return *this;
    }

    backupId_ = other.backupId_;
    directoryPath_ = other.directoryPath_;
    option_ = other.option_;
    version_ = other.version_;

    return *this;
}

Common::Guid BackupInfo::get_BackupId() const
{
    return backupId_;
}

KString::CSPtr BackupInfo::get_Directory() const
{
    return directoryPath_;
}

FABRIC_BACKUP_OPTION BackupInfo::get_BackupOption() const
{
    return option_;
}

BackupVersion BackupInfo::get_BackupVersion() const
{
    return version_;
}
