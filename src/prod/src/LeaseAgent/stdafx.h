// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeaseAgent.h" // list the public header file first to make sure it contains all its dependencies
#include "LeaseAgentEventSource.h"
#include "LeaseConfig.h"
#include "Lease/inc/public/leaselayerpublic.h"
#include "LeasePartner.h"
// Header files needed for compiling cpp files in this library unit
#include "DummyLeaseDriver.h"
#include "Federation/FederationConfig.h"
#include "Transport/TcpTransportUtility.h"
#include "Transport/TransportConfig.h"
#include "limits.h"
