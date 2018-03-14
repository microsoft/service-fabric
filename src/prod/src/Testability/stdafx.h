// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define GET_RC( ResourceName ) \
    Common::StringResource::Get( IDS_TESTABILITY_##ResourceName )

#include "Federation/Federation.h"
#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"
#include "query/Query.h"
#include "query/QueryMessageUtility.h"

#include "Lease/inc/public/leaselayerinc.h"
#include "Lease/inc/public/leaselayerpublic.h"

#include "Hosting2/Hosting.Public.h"
#include "Testability/NodeLevelHealthReportHelper.h"
#include "Testability/UnreliableTransportHelper.h"
#include "Testability/UnreliableLeaseHelper.h"
#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfig.h" 

#include "Testability/TestabilityPointers.h"
#include "Testability/ITestabilitySubsystem.h"
#include "Testability/TestabilityConfig.h"
#include "Testability/TestabilitySubsystem.h"

#include "Testability/Testability.Public.h"
#include "Testability/NodeLevelHealthReportHelper.h"

#include "systemservices/SystemServices.Fabric.h"
#include "FabricNode/fabricnode.h"

#include <fstream>
