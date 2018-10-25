// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfig.h"
#include "TestCommon/TestCommon.h"
#include "TestHooks/TestHooks.h"
#include "FederationTestCommon/FederationTestCommon.h"
#include "query/Query.h"
#include "query/Query.Internal.h"
#include "Reliability/Reliability.h"
#include "Reliability/Failover/TestApi.Failover.h"
#include "Naming/stdafx.h"

#include "Communication/Communication.h"
#include "FabricNode/fabricnode.h"
#include "Reliability/Replication/EnumOperators.h"
//LINUXTODO: use store public interface and avoid ESE specific dependency
#include "Store/stdafx.h"
#include "systemservices/SystemServices.Fabric.h"
#include "Management/ClusterManager/stdafx.h"
#include "Management/healthmanager/HealthManager.Testability.h"
#include "Management/infrastructureservice/InfrastructureService.h"

//Hosting2 includes
#include "Hosting2/Hosting.h"
#include "Hosting2/Hosting.Runtime.h"
#include "Hosting2/HostingPointers.h"

#include "Hosting2/Hosting.Internal.h"
#include "Hosting2/Hosting.Runtime.Internal.h"

#include "Hosting2/Hosting.Protocol.h"
#include "Hosting2/Hosting.Protocol.Internal.h"

#include "Hosting2/Hosting.FabricHost.h"

// FabricClient includes
#include "client/client.h"
#include "client/Client.Internal.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

// HttpGateway
#include "httpgateway/HttpGateway.h"

#include "FabricGateway/FabricGateway.h"

// Transactional replicator includes
// Ensure all fabric test related headers that are KTL specific are included below these
#include <SfStatus.h>
#include "data/txnreplicator/TransactionalReplicator.Public.h"
#include "data/testcommon/TestCommon.Public.h"
#include "data/txnreplicator/testcommon/TransactionalReplicator.TestCommon.Public.h"

// KTL Weirdness
namespace FabricTest
{
    using ::_delete;
}

// tstore includes
#include "data/tstore/Store.Public.h"

//FabricTestCommon headers"
#include "FabricTestCommonConfig.h"
#include "FabricTestConstants.h"
#include "FabricTestCommands.h"
#include "TestMultiPackageHostContext.h"
#include "TestCodePackageContext.h"
#include "MessageType.h"
#include "TestServiceInfo.h"
#include "ServicePlacementData.h"
#include "ClientCommandReply.h"
#include "TestCodePackageData.h"
#include "ComAsyncOperationCallbackTestHelper.h"
#include "ComTestPostBackupHandler.h"
#include "BehaviorStore.h"
#include "ApiFaultHelper.h"
#include "ServiceInitDataParser.h"
#include "PartitionWrapper.h"
#include "ReadWriteStatusValidator.h"
#include "CalculatorService.h"
#include "ITestStoreService.h"
#include "TestStoreService.h"
#include "TestPersistedStoreService.h"
#include "TestStateProvider2.h"
#include "ComTestStateProvider2Factory.h"
#include "TXRService.h"
#include "TStoreService.h"
#include "ComCalculatorService.h"
#include "ComTestReplica.h"
#include "ComTestReplicaMap.h"
#include "ComTestReplicator.h"
#include "ComTestStoreService.h"
#include "ComTestPersistedStoreService.h"
#include "ComTXRService.h"
#include "ComTStoreService.h"

//FabricTest headers
#include "ClientSecurityHelper.h"
#include "ITestScriptableService.h"
#include "ApplicationBuilder.h"
#include "TestStoreServiceProxy.h"
#include "TestStoreClient.h"
#include "TestServiceFactory.h"
#include "ComTestServiceFactory.h"
#include "RandomSessionConfig.h"
#include "SGOperationDataAsyncEnumerator.h"
#include "SGComProxyOperationDataEnumAsyncOperation.h"
#include "SGComProxyOperationStreamAsyncOperation.h"
#include "SGComProxyReplicateAsyncOperation.h"
#include "SGUlonglongOperationData.h"
#include "SGEmptyOperationData.h"
#include "SGStatefulService.h"
#include "SGStatelessService.h"
#include "SGComStatefulService.h"
#include "SGComStatelessService.h"
#include "SGServiceFactory.h"
#include "SGComServiceFactory.h"
#include "SGComCompletedAsyncOperationContext.h"
#include "ServiceLocationChangeCallback.h"
#include "ServiceLocationChangeCacheInfo.h"
#include "ServiceLocationChangeClient.h"
#include "ServiceLocationChangeClientManager.h"
#include "FabricTestRuntimeManager.h"
#include "TestHostingSubsystem.h"
#include "INodeWrapper.h"
#include "FabricNodeWrapper.h"
#include "FabricTestNode.h"
#include "TestFabricUpgradeHostingImpl.h"
#include "FabricTestFederation.h"
#include "TestFabricClientHelperClasses.h"
#include "TestFabricClient.h"
#include "TestWatchDog.h"
#include "CheckpointTruncationTimestampValidator.h"
#include "TestHealthTable.h"
#include "TestFabricClientHealth.h"
#include "TestFabricClientQuery.h"
#include "FabricTestQueryHandlers.h"
#include "FabricTestQueryExecutor.h"
#include "TestFabricClientSettings.h"
#include "TestFabricClientUpgrade.h"
#include "TestFabricClientScenarios.h"
#include "NativeImageStoreExecutor.h"
#include "FabricTestServiceMap.h"
#include "FabricTestApplicationMap.h"
#include "TestUpgradeFabricData.h"
#include "FabricTestNamingState.h"
#include "ConfigurationSetter.h"
#include "FabricTestClientsTracker.h"
#include "FabricTestCommandsTracker.h"
#include "FabricTestVectorExceptionHandler.h"
#include "FabricTestDispatcher.Helper.h"
#include "FabricTestDispatcher.h"
#include "FabricTestSession.h"
#include "FabricTestSessionConfig.h"
#include "RandomTestApplicationHelper.h"
#include "RandomFabricTestSession.h"
#include "TestClientGateway.h"
//FabricTestHost headers
#include "ComCodePackageHost.h"
#include "TestHostServiceFactory.h"
#include "ComTestHostServiceFactory.h"
#include "FabricTestHostDispatcher.h"
#include "FabricTestHostSession.h"
