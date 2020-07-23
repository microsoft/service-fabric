// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include "Common/Common.h"
#include "Federation/Federation.h"
#include "Hosting2/Hosting.h"
#include "Hosting2/NetworkInventoryAgent.h"
#include "Reliability/Reliability.h"
#include "systemservices/SystemServices.Fabric.h"
#include "Naming/naming.h"
#include "Communication/Communication.h"
#include "FabricNode/fabricnode.h"
#include "Management/BackupRestore/BA/BA.h"
#include "Management/Common/ManagementConfig.h"
#include "Hosting2/HostingConfig.h"
#include "client/ClientPointers.h"
#include "Testability/Testability.Public.h"
#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfig.h"
#include "Transport/Throttle.h"
#include "httpgateway/HttpGateway.h"