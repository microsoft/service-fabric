// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TEST_Lock_Context_TAG 'tcLT'
#define TEST_STATE_PROVIDER_TAG 'rpST'
#define TEST_STATE_PROVIDER_FACTORY_TAG 'afST'
#define TEST_STATE_PROVIDER_PROPERTIES_TAG 'ppST'
#define TEST_DATALOSS_HANDLER_TAG 'thDT'
#define TEST_ASYNC_ONDATALOSS_CONTEXT_TAG 'coAT'
#define TEST_COMPROXY_DATALOSS_HANDLER_TAG 'hdCT'
#define TEST_COM_STATE_PROVIDER_FACTORY_TAG 'fpCT'
#define TEST_OPERATION_DATA_STREAM_TAG 'sdOT'
#define TEST_BACKUP_CALLBACK_HANDLER_TAG 'HcBT'

// External dependencies
#include "ktl.h" // SFStatus.h requires
#include <SfStatus.h>
#include "../../utilities/Data.Utilities.Public.h"
#include "../common/TransactionalReplicator.Common.Public.h"
#include "../statemanager/StateManager.Public.h"

// Test Common public
#include "TransactionalReplicator.TestCommon.Public.h"

