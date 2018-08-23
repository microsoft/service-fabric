// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Transport/Transport.h"
#include "api/definitions/ApiDefinitions.h"
#include "Naming/INamingMessageProcessor.h"

//
// Internal fabric client public header files.
//

#include "client/ClientPointers.h"
#include "client/ClientFactory.h"
#include "client/ClientConfig.h"
#include "client/ClientServerTransport/ClientServerRequestMessage.h"
#include "client/ClientServerTransport/ClientServerReplyMessage.h"
#include "client/ClientServerTransport/ClientProtocolVersionHeader.h"

// Health Reporting Component
#include "client/HealthClient.h"
