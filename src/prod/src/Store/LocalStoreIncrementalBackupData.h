// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
	class LocalStoreIncrementalBackupData : public Serialization::FabricSerializable
	{
	public:
		LocalStoreIncrementalBackupData()
            : allowIncrementalBackup_(false)
            , backupChainGuid_(Common::Guid::Empty())
            , prevBackupIndex_(0)            
		{
		}

        LocalStoreIncrementalBackupData(
            bool allowIncrementalBackup, 
            Common::Guid backupChainGuid,
            ULONG prevBackupIndex)
            : allowIncrementalBackup_(allowIncrementalBackup)
            , backupChainGuid_(backupChainGuid)
            , prevBackupIndex_(prevBackupIndex)
		{
		}

        __declspec(property(get = get_IsIncrementalBackupAllowed)) bool const & IsIncrementalBackupAllowed;
        bool const & get_IsIncrementalBackupAllowed() const { return allowIncrementalBackup_; }

        __declspec(property(get = get_BackupChainGuid)) Common::Guid const & BackupChainGuid;
        Common::Guid const & get_BackupChainGuid() const { return backupChainGuid_; }

        __declspec(property(get = get_PreviousBackupIndex)) ULONG const & PreviousBackupIndex;
        ULONG const & get_PreviousBackupIndex() const { return prevBackupIndex_; }

		void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(allowIncrementalBackup_, backupChainGuid_, prevBackupIndex_);

	private:
		bool allowIncrementalBackup_;
        Common::Guid backupChainGuid_;
        ULONG prevBackupIndex_;
	};
}

