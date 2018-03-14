// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Store;

void LocalStoreIncrementalBackupData::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
   w.Write(
        "LocalStoreIncrementalBackupData(Allowed='{0}', FullBackupGuid='{1}', PrevBackupGuid={2})",
        this->allowIncrementalBackup_,
        this->backupChainGuid_,
        this->prevBackupIndex_);
}
