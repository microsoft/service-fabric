// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

//
// Header Files required by internal Runtime header files
//

#include "Hosting2/Hosting.Runtime.h"
#include "Reliability/Failover/Failover.h"
#include "ServiceGroup/Service/ServiceGroup.InternalPublic.h"
#include "FabricClient_.h"
#include "Management/Common/ManagementCommon.h"
#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Lease/inc/public/leaselayerpublic.h"

//
// Internal Runtime header files
//

#include "Hosting2/ApplicationHostCodePackageOperationRequest.h"
#include "Hosting2/ApplicationHostCodePackageOperationReply.h"

#include "Hosting2/ComProxyCodePackageHost.h"
#include "Hosting2/NonActivatedApplicationHost.h"
#include "Hosting2/SingleCodePackageApplicationHost.h"
#include "Hosting2/MultiCodePackageApplicationHost.h"
#include "Hosting2/ComFabricRuntime.h"
#if !defined (PLATFORM_UNIX)
#include "Hosting2/ComFabricBackupRestoreAgent.h"
#endif
#include "Hosting2/LeaseMonitor.h"
#include "Hosting2/ServiceFactoryRegistrationTable.h"
#include "Hosting2/ServiceFactoryRegistration.h"

#include "Hosting2/ApplicationHostCodePackageActivator.h"
#include "Hosting2/SingleCodePackageApplicationHostCodePackageActivator.h"
