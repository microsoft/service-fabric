// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define UNDER_PAL

// External dependencies
#include "../../../Common/Common.h"
#include "FabricCommon.h"
#include "FabricRuntime_.h"
#include "FabricImageStore_.h"
#include "FabricServiceCommunication_.h"
#include "../ReliableCollectionRuntime/dll/ReliableCollectionRuntime.h"
#include "../ReliableCollectionRuntime/dll/ReliableCollectionRuntime.Internal.h"
#include "../../../ServiceModel/ServiceModel.h"


#include "Helpers.h"
#include "EndpointsDescription.h"
#include "StatefulServiceBase.h"
#include "Factory.h"
#include "ComFactory.h"
#include "TestComProxyDataLossHandler.h"
#include "Management/DnsService/config/DnsServiceConfig.h"
