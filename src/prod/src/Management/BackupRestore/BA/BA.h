// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sal.h>
#include <esent.h>

#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Store/store.h"
#include "Federation/Federation.h"
#include "Reliability/Reliability.h"
#include "Transport/TransportFwd.h"
#include "Transport/UnreliableTransport.h"
#include "Transport/IpcReceiverContext.h"
#include "systemservices/IpcFailureBody.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

#include "Management/BackupRestore/BA/BackupPolicyType.h"
#include "Management/BackupRestore/BA/BackupPolicyRunScheduleMode.h"
#include "Management/BackupRestore/BA/BackupPolicyRunFrequencyMode.h"
#include "Management/BackupRestore/BA/BackupStoreType.h"
#include "Management/BackupRestore/BA/BackupStoreFileShareAccessType.h"
#include "Management/BackupRestore/BA/BackupStoreFileShareInfo.h"
#include "Management/BackupRestore/BA/BackupStoreAzureStorageInfo.h"
#include "Management/BackupRestore/BA/BackupStoreDsmsAzureStorageInfo.h"
#include "Management/BackupRestore/BA/BackupStoreInfo.h"
#include "Management/BackupRestore/BA/BackupScheduleRuntimeList.h"
#include "Management/BackupRestore/BA/BackupFrequency.h"
#include "Management/BackupRestore/BA/BackupSchedule.h"
#include "Management/BackupRestore/BA/BackupConfiguration.h"
#include "Management/BackupRestore/BA/BackupPolicy.h"
#include "Management/BackupRestore/BA/RestorePointDetails.h"
#include "Management/BackupRestore/BA/BackupOperationResultMessageBody.h"
#include "Management/BackupRestore/BA/BackupPartitionRequestMessageBody.h"
#include "Management/BackupRestore/BA/RestoreOperationResultMessageBody.h"
#include "Management/BackupRestore/BA/UploadBackupMessageBody.h"
#include "Management/BackupRestore/BA/DownloadBackupMessageBody.h"

#include "Management/BackupRestore/BA/IMessageHandler.h"
#include "Management/BackupRestore/BA/BackupRestorePointers.h"
#include "Management/BackupRestore/BA/BAFederationWrapper.h"
#include "Management/BackupRestore/BA/BackupRestoreAgent.h"
#include "Management/BackupRestore/BA/BAPTransport.h"
#include "Management/BackupRestore/BA/GetPolicyReplyMessageBody.h"
#include "Management/BackupRestore/BA/PartitionInfoMessageBody.h"
#include "Management/BackupRestore/BA/GetRestorePointReplyMessageBody.h"
#include "Management/BackupRestore/BA/BAMessage.h"
#include "Management/BackupRestore/BA/RequestResponseAsyncOperationBase.h"
#include "Management/BackupRestore/BA/NodeToNodeAsyncOperation.h"
#include "Management/BackupRestore/BA/NodeToServiceAsyncOperation.h"
#include "Management/BackupRestoreService/BackupRestoreServiceConfig.h"
#include "Management/BackupRestore/BA/BCP.h"
#include "Management/BackupRestore/BA/BackupCopyAsyncOperation.h"
