// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// Template function to create the wrappers
//

namespace Api
{
    struct WrapperFactory
    {
        #define DEFINE_CREATE_COM_WRAPPER(TInterface, TComInterface, TComWrapperClass)                                                  \
        static Common::ComPointer<TComInterface> create_com_wrapper(__in Common::RootedObjectPointer<TInterface> const & impl)          \
        {                                                                                                                               \
            return Common::make_com<TComWrapperClass, TComInterface>(impl);                                                             \
        }

        #define CREATE_ROOTED_COM_PROXY(TComInterface, TInterface, TComProxyWrapperClass)                                               \
            Common::ComPointer<TComInterface> comImplCPtr;                                                                              \
            comImplCPtr.SetAndAddRef(comImpl);                                                                                          \
                                                                                                                                        \
            auto proxy = std::make_shared<TComProxyWrapperClass>(comImplCPtr);                                                          \
            return Common::RootedObjectPointer<TInterface>((TInterface*) proxy.get(), proxy->CreateComponentRoot());                    \

        #define DEFINE_CREATE_ROOTED_COM_PROXY(TComInterface, TInterface, TComProxyWrapperClass)                                        \
        static Common::RootedObjectPointer<TInterface> create_rooted_com_proxy(__in TComInterface * comImpl)                            \
        {                                                                                                                               \
            if (comImpl == NULL)                                                                                                        \
            {                                                                                                                           \
                return Common::RootedObjectPointer<TInterface>();                                                                       \
            }                                                                                                                           \
                                                                                                                                        \
            CREATE_ROOTED_COM_PROXY(TComInterface, TInterface, TComProxyWrapperClass)                                                   \
        }

        #define DEFINE_CREATE_COM_PROXY_WRAPPER(TComInterface, TComWrapperClass, TInterface, TComProxyWrapperClass)                     \
        static Common::RootedObjectPointer<TInterface> create_com_proxy_wrapper(__in TComInterface * comImpl)                           \
        {                                                                                                                               \
            if (comImpl == NULL)                                                                                                        \
            {                                                                                                                           \
                return Common::RootedObjectPointer<TInterface>();                                                                       \
            }                                                                                                                           \
                                                                                                                                        \
            Common::ComPointer<TComWrapperClass> comWrapper(comImpl, CLSID_##TComWrapperClass);                                         \
            if (comWrapper)                                                                                                             \
            {                                                                                                                           \
                return comWrapper->get_Impl();                                                                                          \
            }                                                                                                                           \
            else                                                                                                                        \
            {                                                                                                                           \
                CREATE_ROOTED_COM_PROXY(TComInterface, TInterface, TComProxyWrapperClass)                                               \
            }                                                                                                                           \
        }

        //
        // COM wrapper creation methods
        //
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreItemEnumerator, IFabricKeyValueStoreItemEnumerator, ComKeyValueStoreItemEnumerator)
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreItemMetadataEnumerator, IFabricKeyValueStoreItemMetadataEnumerator, ComKeyValueStoreItemMetadataEnumerator)
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreItemMetadataResult, IFabricKeyValueStoreItemMetadataResult, ComKeyValueStoreItemMetadataResult)
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreItemResult, IFabricKeyValueStoreItemResult, ComKeyValueStoreItemResult)
        DEFINE_CREATE_COM_WRAPPER(IStoreEventHandler, IFabricStoreEventHandler, ComStoreEventHandler )
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreReplica, IFabricKeyValueStoreReplica, ComKeyValueStoreReplica)
        DEFINE_CREATE_COM_WRAPPER(IStateProvider, IFabricStateProvider, ComStateProvider)
        DEFINE_CREATE_COM_WRAPPER(IStatefulServiceFactory, IFabricStatefulServiceFactory, ComStatefulServiceFactory)
        DEFINE_CREATE_COM_WRAPPER(IStatefulServiceReplica, IFabricStatefulServiceReplica, ComStatefulServiceReplica)
        DEFINE_CREATE_COM_WRAPPER(IStatelessServiceFactory, IFabricStatelessServiceFactory, ComStatelessServiceFactory)
        DEFINE_CREATE_COM_WRAPPER(IStatelessServiceInstance, IFabricStatelessServiceInstance, ComStatelessServiceInstance)
        DEFINE_CREATE_COM_WRAPPER(ITransaction, IFabricTransaction, ComTransaction)
        DEFINE_CREATE_COM_WRAPPER(ITransactionBase, IFabricTransactionBase, ComTransactionBase)
        DEFINE_CREATE_COM_WRAPPER(IInfrastructureServiceAgent, IFabricInfrastructureServiceAgent, ComInfrastructureServiceAgent)
        DEFINE_CREATE_COM_WRAPPER(IFaultAnalysisServiceAgent, IFabricFaultAnalysisServiceAgent, ComFaultAnalysisServiceAgent)
        DEFINE_CREATE_COM_WRAPPER(IBackupRestoreServiceAgent, IFabricBackupRestoreServiceAgent, ComBackupRestoreServiceAgent)
        DEFINE_CREATE_COM_WRAPPER(ITokenValidationServiceAgent, IFabricTokenValidationServiceAgent, ComTokenValidationServiceAgent)
        DEFINE_CREATE_COM_WRAPPER(IStoreEnumerator, IFabricKeyValueStoreEnumerator, ComKeyValueStoreEnumerator)
        DEFINE_CREATE_COM_WRAPPER(IStoreItemEnumerator, IFabricKeyValueStoreItemEnumerator, ComStoreItemEnumerator)
        DEFINE_CREATE_COM_WRAPPER(IStoreItemMetadataEnumerator, IFabricKeyValueStoreItemMetadataEnumerator, ComStoreItemMetadataEnumerator)
        DEFINE_CREATE_COM_WRAPPER(IStoreNotificationEnumerator, IFabricKeyValueStoreNotificationEnumerator, ComKeyValueStoreNotificationEnumerator)
        DEFINE_CREATE_COM_WRAPPER(IStoreItem, IFabricKeyValueStoreNotification, ComKeyValueStoreNotification)
        DEFINE_CREATE_COM_WRAPPER(INativeImageStoreClient, IFabricNativeImageStoreClient, ComNativeImageStoreClient)
        DEFINE_CREATE_COM_WRAPPER(IServiceEndpointsVersion, IFabricServiceEndpointsVersion, ComServiceEndpointsVersion)
        DEFINE_CREATE_COM_WRAPPER(IServiceNotification, IFabricServiceNotification, ComServiceNotification)
        DEFINE_CREATE_COM_WRAPPER(IEseLocalStoreSettingsResult, IFabricEseLocalStoreSettingsResult, ComEseLocalStoreSettingsResult)
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreReplicaSettingsResult, IFabricKeyValueStoreReplicaSettingsResult, ComKeyValueStoreReplicaSettingsResult)
        DEFINE_CREATE_COM_WRAPPER(IKeyValueStoreReplicaSettings_V2Result, IFabricKeyValueStoreReplicaSettings_V2Result, ComKeyValueStoreReplicaSettings_V2Result)
        DEFINE_CREATE_COM_WRAPPER(ISharedLogSettingsResult, IFabricSharedLogSettingsResult, ComSharedLogSettingsResult)
        DEFINE_CREATE_COM_WRAPPER(IServiceCommunicationListener, IFabricServiceCommunicationListener, ComFabricServiceCommunicationListener)
        DEFINE_CREATE_COM_WRAPPER(IServiceCommunicationClient, IFabricServiceCommunicationClient, ComFabricServiceCommunicationClient)
        DEFINE_CREATE_COM_WRAPPER(IClientConnection, IFabricTransportClientConnection, ComFabricTransportClientConnection)
        DEFINE_CREATE_COM_WRAPPER(IStatefulServiceReplicaStatusResult, IFabricStatefulServiceReplicaStatusResult, ComStatefulServiceReplicaStatusResult)
        DEFINE_CREATE_COM_WRAPPER(IUpgradeOrchestrationServiceAgent, IFabricUpgradeOrchestrationServiceAgent, ComUpgradeOrchestrationServiceAgent)
        DEFINE_CREATE_COM_WRAPPER(IContainerActivatorServiceAgent, IFabricContainerActivatorServiceAgent2, ComContainerActivatorServiceAgent)
        DEFINE_CREATE_COM_WRAPPER(IGatewayResourceManagerAgent, IFabricGatewayResourceManagerAgent, ComGatewayResourceManagerAgent)

        //
        // COM proxy wrapper creation methods
        //
        DEFINE_CREATE_COM_PROXY_WRAPPER(IFabricStatelessServiceFactory, ComStatelessServiceFactory, IStatelessServiceFactory, ComProxyStatelessServiceFactory)
        DEFINE_CREATE_COM_PROXY_WRAPPER(IFabricTransactionBase, ComTransactionBase, ITransactionBase, ComProxyTransactionBase)
        DEFINE_CREATE_COM_PROXY_WRAPPER(IFabricTransaction, ComTransaction, ITransaction, ComProxyTransaction)
        DEFINE_CREATE_COM_PROXY_WRAPPER(IFabricStoreEventHandler, ComStoreEventHandler, IStoreEventHandler, ComProxyStoreEventHandler)
        DEFINE_CREATE_COM_PROXY_WRAPPER(IFabricStoreEventHandler2, ComStoreEventHandler , IStoreEventHandler, ComProxyStoreEventHandler)

        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricInfrastructureService, IInfrastructureService, ComProxyInfrastructureService)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricFaultAnalysisService, IFaultAnalysisService, ComProxyFaultAnalysisService)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricBackupRestoreService, IBackupRestoreService, ComProxyBackupRestoreService)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricBackupRestoreHandler, IBackupRestoreHandler, ComProxyBackupRestoreHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricTokenValidationService, ITokenValidationService, ComProxyTokenValidationService)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricSecondaryEventHandler, ISecondaryEventHandler, ComProxySecondaryEventHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricServiceNotificationEventHandler, IServiceNotificationEventHandler, ComProxyServiceNotificationEventHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricServiceConnectionHandler, IServiceConnectionHandler, ComProxyServiceConnectionHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricTransportConnectionHandler, IServiceConnectionHandler, ComProxyFabricTransportConnectionHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricClientConnectionEventHandler, IClientConnectionEventHandler, ComProxyClientConnectionEventHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricStorePostBackupHandler, IStorePostBackupHandler, ComProxyStorePostBackupHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricCommunicationMessageHandler, IServiceCommunicationMessageHandler, ComProxyServiceCommunicationMessageHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricTransportCallbackMessageHandler, IServiceCommunicationMessageHandler, ComProxyFabricTransportCallbackMessageHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricTransportClientEventHandler, IServiceConnectionEventHandler, ComProxyFabricTransportClientEventHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricServiceConnectionEventHandler, IServiceConnectionEventHandler, ComProxyServiceConnectionEventHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricUpgradeOrchestrationService, IUpgradeOrchestrationService, ComProxyUpgradeOrchestrationService)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricNativeImageStoreProgressEventHandler, INativeImageStoreProgressEventHandler, ComProxyNativeImageStoreProgressEventHandler)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricContainerActivatorService2, IContainerActivatorService, ComProxyContainerActivatorService)
        DEFINE_CREATE_ROOTED_COM_PROXY(IFabricGatewayResourceManager, IGatewayResourceManager, ComProxyGatewayResourceManager)
    };
}
