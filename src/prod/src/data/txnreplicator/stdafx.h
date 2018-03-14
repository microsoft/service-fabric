// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TRANSACTIONALREPLICATOR_TAG 'LRXT'
#define COM_PROXY_DATALOSS_HANDLER_TAG 'hdPC'
#define COM_PROXY_STATE_PROVIDER_FACTORY 'fsPC'
#define RUNTIMEFOLDERS_TAG 'FnuR'

// Public headers.
#include "TransactionalReplicator.Public.h"
#include "../logicallog/LogicalLog.Public.h" // SFStatus.h requires ktl.h

#include <SfStatus.h>

// External dependencies
#include "./common/TransactionalReplicator.Common.Internal.h"
#include "./statemanager/StateManager.Internal.h"
#include "./loggingreplicator/LoggingReplicator.Internal.h"

#include "RuntimeFolders.h"
#include "TransactionalReplicator.h"
#include "ComProxyDataLossHandler.h"
#include "ComOperationDataStream.h"
#include "ComProxyOperation.h"
#include "ComProxyOperationDataStream.h"
#include "ComProxyOperationStream.h"
#include "ComStateProvider.h"
#include "ComProxyStateReplicator.h"
#include "ComTransactionalReplicator.h"
#include "ComTransactionalReplicatorFactory.h"
#include "ComProxyStateProvider2Factory.h"

