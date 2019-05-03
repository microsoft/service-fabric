// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//Fabric includes -- Transport.h brings in a lot of the standard Fabric includes
#include "Transport\Transport.h"
#include "Common\CommonEventSource.h"

// KTL includes
#include "ktl.h"
#include "Common\KtlSF.Common.h"
#include "KUnknown.h"
#include "AsyncCallInAdapter.h"
#include "AsyncCallOutAdapter.h"
#include "FabricAsyncServiceBase.h"
#include "FabricAsyncDeferredClosureServiceBase.h" // private derivation for now

// IDL generated includes
#include "fabricreliablemessaging_.h"
#include "fabrictypes.h"

using Common::ErrorCode;
using Common::ComPointer;

#define COM_INTEROP_ALLOCATION_TAG 'CIAT'
#define RMSG_ALLOCATION_TAG 'RMSG'

#include "ReliableMessagingEventSource.h"
#include "ReliableMessagingMacros.h"
#include "ReliableMessagingActions.h"
#include "ReliableMessagingConstants.h"
#include "ReliableMessagingDataStructures.h"
#include "ReliableMessagingTypes.h"
#include "ReliableMessagingPartitionKey.h"
#include "ReliableMessagingHeaders.h"
#include "ReliableMessagingAsyncQueue.h"
#include "ReliableMessagingTransport.h"
#include "ReliableMessagingServicePartition.h"
#include "ReliableMessageId.h"
#include "ReliableSessionRingBuffer.h"
#include "ReliableSessionService.h"
#include "ReliableSessionAsyncOperations.h"
#include "ReliableSessionOpenCloseOperations.h"
#include "ReliableSessionCallbacks.h"
#include "SessionManagerAsyncOperations.h"
#include "ReliableMessagingTwoTierIndex.h"
#include "ReliableSessionManager.h"
#include "ReliableMessagingEnvironment.h"
#include "ComReliableMessaging.h"

