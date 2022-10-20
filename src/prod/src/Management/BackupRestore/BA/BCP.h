// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Federation/Federation.h"

#include "Management/BackupRestore/BA/BackupCopierAsyncOperationBase.h"
#include "Management/BackupRestore/BA/RunBackupCopierExeAsyncOperation.h"
#include "Management/BackupRestore/BA/AzureStorageBackupCopierAsyncOperationBase.h"
#include "Management/BackupRestore/BA/AzureStorageUploadBackupAsyncOperation.h"
#include "Management/BackupRestore/BA/AzureStorageDownloadBackupAsyncOperation.h"
#include "Management/BackupRestore/BA/DsmsAzureStorageBackupCopierAsyncOperationBase.h"
#include "Management/BackupRestore/BA/DsmsAzureStorageUploadBackupAsyncOperation.h"
#include "Management/BackupRestore/BA/DsmsAzureStorageDownloadBackupAsyncOperation.h"
#include "Management/BackupRestore/BA/FileShareBackupCopierAsyncOperationBase.h"
#include "Management/BackupRestore/BA/FileShareUploadBackupAsyncOperation.h"
#include "Management/BackupRestore/BA/FileShareDownloadBackupAsyncOperation.h"
#include "Management/BackupRestore/BA/BackupCopierJobItem.h"
#include "Management/BackupRestore/BA/BackupCopierJobQueueBase.h"
#include "Management/BackupRestore/BA/UploadBackupJobQueue.h"
#include "Management/BackupRestore/BA/DownloadBackupJobQueue.h"

#include "Management/BackupRestore/BA/BackupCopierProxy.h"
