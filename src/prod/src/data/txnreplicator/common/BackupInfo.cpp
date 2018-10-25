// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Utilities;
using namespace TxnReplicator;

BackupInfo::BackupInfo() noexcept
    : KObject()
    , directoryPath_()
    , option_(FABRIC_BACKUP_OPTION_INVALID)
    , version_(BackupVersion::Invalid()) 
    , startVersion_(BackupVersion::Invalid())
    , backupId_()
    , parentBackupId_()
{
}

BackupInfo::BackupInfo(
    __in KString const & directoryPath,
    __in FABRIC_BACKUP_OPTION option,
    __in BackupVersion version,
    __in BackupVersion startVersion,
    __in Common::Guid const & backupId,
    __in Common::Guid const & parentBackupId,
    __in KAllocator & allocator) noexcept
    : KObject()
    , backupId_(backupId)
    , parentBackupId_(parentBackupId)
    , option_(option)
    , version_(version)
    , startVersion_(startVersion)
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
        directoryPath_ = other.directoryPath_;
        option_ = other.option_;
        version_ = other.version_;
        startVersion_ = other.startVersion_;
        backupId_ = other.backupId_;
        parentBackupId_ = other.parentBackupId_;
    }
}

BackupInfo & BackupInfo::operator=(
    __in BackupInfo const & other) noexcept
{
    if (&other == this)
    {
        return *this;
    }

    directoryPath_ = other.directoryPath_;
    option_ = other.option_;
    version_ = other.version_;
    startVersion_ = other.startVersion_;
    backupId_ = other.backupId_;
    parentBackupId_ = other.parentBackupId_;

    return *this;
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

BackupVersion BackupInfo::get_BackupStartVersion() const
{
    return startVersion_;
}

Common::Guid BackupInfo::get_BackupId() const
{
    return backupId_;
}

Common::Guid BackupInfo::get_ParentBackupId() const
{
    return parentBackupId_;
}