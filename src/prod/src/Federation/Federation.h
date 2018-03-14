// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <stdio.h>

#include "Federation/FederationPointers.h"

#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Store/Store.Public.h"

#include "ServiceModel/federation/NodeInstance.h"
#include "ServiceModel/federation/NodeId.h"
#include "ServiceModel/federation/NodeIdGenerator.h"

#include "client/ClientPointers.h"

#include "Federation/Types.h"
#include "Federation/IMessageFilter.h"

#include "Federation/Constants.h"
#include "Federation/NodeConfig.h"
#include "Federation/NodeIdRange.h"
#include "Federation/NodePhase.h"
#include "Federation/NodeTraceComponent.h"
#include "Federation/NodeActivityTraceComponent.h"

#include "Federation/RoutingToken.h"
#include "Federation/IRouter.h"
#include "Federation/IMultipleReplyContext.h"
#include "Federation/ReceiverContext.h"
#include "Federation/OneWayReceiverContext.h"
#include "Federation/RequestReceiverContext.h"
#include "Federation/MessageHandlerPair.h"
#include "Federation/PartnerNode.h"

#include "Federation/SerializableWithActivation.h"
#include "Federation/SerializableActivationFactory.h"

#include "Federation/GlobalStoreProvider.h"

#include "Federation/FederationSubsystem.h"
