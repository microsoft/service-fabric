// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

namespace Store
{
    namespace StoreBackupType
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case StoreBackupType::None: w << "None"; return;
            case StoreBackupType::Full: w << "Full"; return;
            case StoreBackupType::Incremental: w << "Incremental"; return;
            default: w << "StoreBackupType(" << static_cast<ULONG>(e) << ')'; return;
            }
        }
    }

    GlobalWString BackupRestoreData::FileName = make_global<wstring>(L"restore.dat");

    void BackupRestoreData::WriteTo(TextWriter& w, FormatOptions const &) const
    {
		wstring backupFilesStr;
		for (auto const & file : this->backupFiles_)
		{
			backupFilesStr.append(file + L";");
		}

		backupFilesStr.erase(--backupFilesStr.end());

        w.Write(
			"BackupRestoreData(PartitionInfo=[{0}], BackupType=[{1}], BackupIndex=[{2}], BackupChainFullBackupGuid=[{3}], ReplicaId=[{4}], TimeStampUtc=[{5}], BackupFiles=[{6}]).",
            this->partitionInfo_,
            this->backupType_,
            this->backupIndex_,
            this->backupChainGuid_,
            this->replicaId_,
            this->timeStampUtc_,
			backupFilesStr);
    }
}
