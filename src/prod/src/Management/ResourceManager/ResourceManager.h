// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Federation/Federation.h"
#include "Store/store.h"
#include "ServiceModel/ServiceModel.h"
#include "Naming/naming.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

#include "Constants.h"
#include "ResourceDataItem.h"
#include "ResourceManagerCore.h"
#include "Context.h"

#include "ReplicaActivityAsyncOperation.h"
#include "AutoRetryAsyncOperation.h"
#include "ResourceManagerAsyncOperation.h"
#include "ClaimBasedAsyncOperation.h"
#include "ReleaseResourceAsyncOperation.h"
#include "ClaimResourceAsyncOperation.h"
#include "ResourceManagerService.h"