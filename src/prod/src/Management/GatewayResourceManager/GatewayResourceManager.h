// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Federation/Federation.h"
#include "query/Query.h"
#include "api/wrappers/ComGatewayResourceManagerAgentFactory.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

#include "api/definitions/ApiDefinitions.h"
#include "Hosting2/Hosting.Runtime.Internal.h"
#include "systemservices/SystemServices.Service.h"

#include "Management/GatewayResourceManager/Constants.h"
#include "Management/GatewayResourceManager/GatewayResourceManagerConfig.h"
#include "Management/GatewayResourceManager/GatewayResourceManagerMessage.h"
#include "Management/GatewayResourceManager/GatewayResourceManagerAgent.h"
#include "Management/GatewayResourceManager/GatewayResourceManagerAgentFactory.h"
#include "Management/GatewayResourceManager/ProcessQueryAsyncOperation.h"
