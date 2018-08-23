// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

//
// External Header Files required by public Hosting header files
//

#include "LeaseAgent/LeaseAgent.h"
#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Management/Common/ManagementCommon.h"
#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Reliability/Failover/FailoverPointers.h"
#if !defined (PLATFORM_UNIX)
#include "Management/BackupRestore/BA/BAP.h"
#endif
#include "client/client.h"
#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricRuntime_.h"
#include "data/logicallog/LogicalLog.Public.h"

//
// Public Runtime header files
//

#include "Hosting2/HostingPointers.h"
#include "Hosting2/Hosting.RuntimePointers.h"
#include "Hosting2/Hosting.Runtime.Public.h"
#include "Hosting2/ApplicationHostType.h"
#include "Hosting2/NodeHostClosedEventArgs.h"
#include "Hosting2/ApplicationHostContext.h"
#include "Hosting2/CodePackageContext.h"
#include "Hosting2/FabricNodeContext.h"
#include "Hosting2/ApplicationHost.h"
#include "Hosting2/ServiceFactoryManager.h"
#include "Hosting2/ComProxyStatelessServiceFactory.h"
#include "Hosting2/ComProxyStatefulServiceFactory.h"
#if !defined (PLATFORM_UNIX)
#include "Hosting2/FabricBackupRestoreAgentImpl.h"
#endif
#include "Hosting2/FabricRuntimeContext.h"
#include "Hosting2/FabricRuntimeImpl.h"
#include "Hosting2/RuntimeSharingHelper.h"
#include "Hosting2/ITentativeCodePackageActivationContext.h"
#include "Hosting2/PackageChangeHandler.h"
#include "Hosting2/ComCodePackage.h"
#include "Hosting2/ComConfigPackage.h"
#include "Hosting2/ComDataPackage.h"
#include "Hosting2/CodePackageActivationContext.h"
#include "Hosting2/ComCodePackageActivationContext.h"
#include "Hosting2/ITentativeApplicationHostCodePackageActivator.h"
#include "Hosting2/ComApplicationHostCodePackageActivator.h"
#if !defined (PLATFORM_UNIX)
#include "Hosting2/FabricGetBackupSchedulePolicyResult.h"
#include "Hosting2/FabricGetRestorePointDetailsResult.h"
#endif
#include "Hosting2/FabricNodeContextResult.h"
#include "Hosting2/ComFabricNodeContextResult.h"
#if !defined (PLATFORM_UNIX)
#include "Hosting2/ComFabricGetBackupSchedulePolicyResult.h"
#include "Hosting2/ComFabricGetRestorePointDetailsResult.h"
#endif
#include "Hosting2/ApplicationHostContainer.h"
#include "Hosting2/NetworkContextType.h"

