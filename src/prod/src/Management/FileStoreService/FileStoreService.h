// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

//
// External Header Files required by public ImageModel header files
//
#define _CRT_RAND_S
#include "stdlib.h"
#include <string>

#include "Common/Common.h"
#include "Management/ImageModel/ImageModel.h"
#include "ServiceModel/ServiceModel.h"
#include "Transport/Transport.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"
#include "systemservices/SystemServices.Service.h"

//
// Common header files
//
#include "Management/FileStoreService/FileStorePointers.h"
#include "Management/FileStoreService/Constants.h"
#include "Management/FileStoreService/FileStoreServiceConfig.h"
#include "Management/FileStoreService/VersionNumber.h"
#include "Management/FileStoreService/Utility.h"
#include "Management/FileStoreService/FileStoreServiceMessage.h"
#include "Management/FileStoreService/FileStoreServiceMessage.h"
#include "Management/FileStoreService/FileStorePartitionInfo.h"
#include "Management/FileStoreService/ImpersonatedSMBCopyContext.h"

//
// Client header files
//
#include "Management/FileStoreService/IFileStoreClient.h"
#include "Management/FileStoreService/StagingLocationInfo.h"
#include "Management/FileStoreService/PartitionContext.h"
#include "Management/FileStoreService/InternalFileStoreClient.h"
#include "Management/FileStoreService/FileStoreClient.h"

//
// Service header files
//
#include "Management/FileStoreService/PendingWriteOperations.h"
#include "Management/FileStoreService/CopyDescription.h"
#include "Management/FileStoreService/FileMetadata.h"
#include "Management/FileStoreService/ReplicatedStoreWrapper.h"
#include "Management/FileStoreService/UploadSessionMetadata.h"
#include "Management/FileStoreService/uploadsessionmap.h"
#include "Management/FileStoreService/StoreTransactionAsyncOperation.h"
#include "Management/FileStoreService/RequestManager.h"
#include "Management/FileStoreService/ProcessRequestAsyncOperation.h"
#include "Management/FileStoreService/ProcessDeleteRequestAsyncOperation.h"
#include "Management/FileStoreService/ProcessUploadRequestAsyncOperation.h"
#include "Management/FileStoreService/ProcessCopyRequestAsyncOperation.h"
#include "Management/FileStoreService/ProcessListRequestAsyncOperation.h"
#include "Management/FileStoreService/ProcessInternalListRequestAsyncOperation.h"
#include "Management/FileStoreService/processlistuploadsessionrequestasyncoperation.h"
#include "Management/FileStoreService/processdeleteuploadsessionrequestasyncoperation.h"
#include "Management/FileStoreService/processcommituploadsessionrequestasyncoperation.h"
#include "Management/FileStoreService/processcreateuploadsessionrequestasyncoperation.h"
#include "Management/FileStoreService/processuploadchunkrequestasyncoperation.h"
#include "Management/FileStoreService/processcheckexistencerequestasyncoperation.h"
#include "Management/FileStoreService/RequestManager.h"
#include "Management/FileStoreService/ReplicationManager.h"
#include "Management/FileStoreService/FileStoreServiceFactory.h"
#include "Management/FileStoreService/FileStoreServiceReplica.h"
#include "Management/FileStoreService/FileAsyncOperation.h"
#include "Management/FileStoreService/FileUploadAsyncOperation.h"
#include "Management/FileStoreService/FileCopyAsyncOperation.h"
#include "Management/FileStoreService/FileDeleteAsyncOperation.h"
#include "Management/FileStoreService/processuploadchunkcontentrequestasyncoperation.h"
#include "Management/FileStoreService/FileStoreService.PerformanceCounters.h"
