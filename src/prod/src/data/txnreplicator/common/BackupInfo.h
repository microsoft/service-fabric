// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class BackupInfo final
        : public KObject<BackupInfo>
    {
    public:
        __declspec(property(get = get_BackupId)) Common::Guid BackupId;
        Common::Guid get_BackupId() const;

        __declspec(property(get = get_Directory)) KString::CSPtr Directory;
        KString::CSPtr get_Directory() const;

        __declspec(property(get = get_BackupOption)) FABRIC_BACKUP_OPTION Option;
        FABRIC_BACKUP_OPTION get_BackupOption() const;

        __declspec(property(get = get_BackupVersion)) BackupVersion Version;
        BackupVersion get_BackupVersion() const;

    public:
        BackupInfo() noexcept;

        BackupInfo(
            __in Common::Guid const & backupId,
            __in KString const & directoryPath,
            __in FABRIC_BACKUP_OPTION option,
            __in BackupVersion version,
            __in KAllocator & allocator) noexcept;

        BackupInfo(
            __in BackupInfo const & other) noexcept;

        BackupInfo & operator=(
            __in const BackupInfo & other) noexcept;

    private:
        Common::Guid backupId_;
        KString::CSPtr directoryPath_;
        FABRIC_BACKUP_OPTION option_;
        BackupVersion version_;
    };
}
