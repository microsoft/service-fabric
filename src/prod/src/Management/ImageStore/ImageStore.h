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
#include "api/definitions/ApiDefinitions.h"
#include "Transport/Transport.h"
#include "Federation/Federation.h"
#include "query/Query.h"
#include "Reliability/Reliability.h"
#include "ServiceModel/ServiceModel.h"
#include "ServiceModel/SchemaNames.h"
#include "Store/store.h"
#include "systemservices/SystemServices.Service.h"
#include "Management/FileStoreService/FileStoreService.h"

//
// ImageStore public header files
//
#include "Management/ImageStore/ImageStoreServiceConfig.h"
#include "Management/ImageStore/ImageStoreAccessDescription.h"
#include "Management/ImageStore/ImageStoreUtility.h"
#include "Management/ImageStore/IImageStore.h"
#include "Management/ImageStore/ImageCacheConstants.h"
#include "Management/ImageStore/FileImageStore.h"
#include "Management/ImageStore/AzureImageStore.h"
#include "Management/ImageStore/NativeImageStoreProgressState.h"
#include "Management/ImageStore/NativeImageStore.h"
#include "Management/ImageStore/ImageStoreFactory.h"
