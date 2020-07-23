

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0613 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __fabricruntime_h__
#define __fabricruntime_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricRuntime_FWD_DEFINED__
#define __IFabricRuntime_FWD_DEFINED__
typedef interface IFabricRuntime IFabricRuntime;

#endif 	/* __IFabricRuntime_FWD_DEFINED__ */


#ifndef __IFabricProcessExitHandler_FWD_DEFINED__
#define __IFabricProcessExitHandler_FWD_DEFINED__
typedef interface IFabricProcessExitHandler IFabricProcessExitHandler;

#endif 	/* __IFabricProcessExitHandler_FWD_DEFINED__ */


#ifndef __IFabricStatelessServiceFactory_FWD_DEFINED__
#define __IFabricStatelessServiceFactory_FWD_DEFINED__
typedef interface IFabricStatelessServiceFactory IFabricStatelessServiceFactory;

#endif 	/* __IFabricStatelessServiceFactory_FWD_DEFINED__ */


#ifndef __IFabricStatelessServiceInstance_FWD_DEFINED__
#define __IFabricStatelessServiceInstance_FWD_DEFINED__
typedef interface IFabricStatelessServiceInstance IFabricStatelessServiceInstance;

#endif 	/* __IFabricStatelessServiceInstance_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition_FWD_DEFINED__
#define __IFabricStatelessServicePartition_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition IFabricStatelessServicePartition;

#endif 	/* __IFabricStatelessServicePartition_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition1_FWD_DEFINED__
#define __IFabricStatelessServicePartition1_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition1 IFabricStatelessServicePartition1;

#endif 	/* __IFabricStatelessServicePartition1_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition2_FWD_DEFINED__
#define __IFabricStatelessServicePartition2_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition2 IFabricStatelessServicePartition2;

#endif 	/* __IFabricStatelessServicePartition2_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition3_FWD_DEFINED__
#define __IFabricStatelessServicePartition3_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition3 IFabricStatelessServicePartition3;

#endif 	/* __IFabricStatelessServicePartition3_FWD_DEFINED__ */


#ifndef __IFabricStatefulServiceFactory_FWD_DEFINED__
#define __IFabricStatefulServiceFactory_FWD_DEFINED__
typedef interface IFabricStatefulServiceFactory IFabricStatefulServiceFactory;

#endif 	/* __IFabricStatefulServiceFactory_FWD_DEFINED__ */


#ifndef __IFabricStatefulServiceReplica_FWD_DEFINED__
#define __IFabricStatefulServiceReplica_FWD_DEFINED__
typedef interface IFabricStatefulServiceReplica IFabricStatefulServiceReplica;

#endif 	/* __IFabricStatefulServiceReplica_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition_FWD_DEFINED__
#define __IFabricStatefulServicePartition_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition IFabricStatefulServicePartition;

#endif 	/* __IFabricStatefulServicePartition_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition1_FWD_DEFINED__
#define __IFabricStatefulServicePartition1_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition1 IFabricStatefulServicePartition1;

#endif 	/* __IFabricStatefulServicePartition1_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition2_FWD_DEFINED__
#define __IFabricStatefulServicePartition2_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition2 IFabricStatefulServicePartition2;

#endif 	/* __IFabricStatefulServicePartition2_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition3_FWD_DEFINED__
#define __IFabricStatefulServicePartition3_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition3 IFabricStatefulServicePartition3;

#endif 	/* __IFabricStatefulServicePartition3_FWD_DEFINED__ */


#ifndef __IFabricStateProvider_FWD_DEFINED__
#define __IFabricStateProvider_FWD_DEFINED__
typedef interface IFabricStateProvider IFabricStateProvider;

#endif 	/* __IFabricStateProvider_FWD_DEFINED__ */


#ifndef __IFabricStateReplicator_FWD_DEFINED__
#define __IFabricStateReplicator_FWD_DEFINED__
typedef interface IFabricStateReplicator IFabricStateReplicator;

#endif 	/* __IFabricStateReplicator_FWD_DEFINED__ */


#ifndef __IFabricReplicator_FWD_DEFINED__
#define __IFabricReplicator_FWD_DEFINED__
typedef interface IFabricReplicator IFabricReplicator;

#endif 	/* __IFabricReplicator_FWD_DEFINED__ */


#ifndef __IFabricPrimaryReplicator_FWD_DEFINED__
#define __IFabricPrimaryReplicator_FWD_DEFINED__
typedef interface IFabricPrimaryReplicator IFabricPrimaryReplicator;

#endif 	/* __IFabricPrimaryReplicator_FWD_DEFINED__ */


#ifndef __IFabricReplicatorCatchupSpecificQuorum_FWD_DEFINED__
#define __IFabricReplicatorCatchupSpecificQuorum_FWD_DEFINED__
typedef interface IFabricReplicatorCatchupSpecificQuorum IFabricReplicatorCatchupSpecificQuorum;

#endif 	/* __IFabricReplicatorCatchupSpecificQuorum_FWD_DEFINED__ */


#ifndef __IFabricOperation_FWD_DEFINED__
#define __IFabricOperation_FWD_DEFINED__
typedef interface IFabricOperation IFabricOperation;

#endif 	/* __IFabricOperation_FWD_DEFINED__ */


#ifndef __IFabricOperationData_FWD_DEFINED__
#define __IFabricOperationData_FWD_DEFINED__
typedef interface IFabricOperationData IFabricOperationData;

#endif 	/* __IFabricOperationData_FWD_DEFINED__ */


#ifndef __IFabricOperationStream_FWD_DEFINED__
#define __IFabricOperationStream_FWD_DEFINED__
typedef interface IFabricOperationStream IFabricOperationStream;

#endif 	/* __IFabricOperationStream_FWD_DEFINED__ */


#ifndef __IFabricOperationStream2_FWD_DEFINED__
#define __IFabricOperationStream2_FWD_DEFINED__
typedef interface IFabricOperationStream2 IFabricOperationStream2;

#endif 	/* __IFabricOperationStream2_FWD_DEFINED__ */


#ifndef __IFabricOperationDataStream_FWD_DEFINED__
#define __IFabricOperationDataStream_FWD_DEFINED__
typedef interface IFabricOperationDataStream IFabricOperationDataStream;

#endif 	/* __IFabricOperationDataStream_FWD_DEFINED__ */


#ifndef __IFabricAtomicGroupStateProvider_FWD_DEFINED__
#define __IFabricAtomicGroupStateProvider_FWD_DEFINED__
typedef interface IFabricAtomicGroupStateProvider IFabricAtomicGroupStateProvider;

#endif 	/* __IFabricAtomicGroupStateProvider_FWD_DEFINED__ */


#ifndef __IFabricAtomicGroupStateReplicator_FWD_DEFINED__
#define __IFabricAtomicGroupStateReplicator_FWD_DEFINED__
typedef interface IFabricAtomicGroupStateReplicator IFabricAtomicGroupStateReplicator;

#endif 	/* __IFabricAtomicGroupStateReplicator_FWD_DEFINED__ */


#ifndef __IFabricServiceGroupFactory_FWD_DEFINED__
#define __IFabricServiceGroupFactory_FWD_DEFINED__
typedef interface IFabricServiceGroupFactory IFabricServiceGroupFactory;

#endif 	/* __IFabricServiceGroupFactory_FWD_DEFINED__ */


#ifndef __IFabricServiceGroupFactoryBuilder_FWD_DEFINED__
#define __IFabricServiceGroupFactoryBuilder_FWD_DEFINED__
typedef interface IFabricServiceGroupFactoryBuilder IFabricServiceGroupFactoryBuilder;

#endif 	/* __IFabricServiceGroupFactoryBuilder_FWD_DEFINED__ */


#ifndef __IFabricServiceGroupPartition_FWD_DEFINED__
#define __IFabricServiceGroupPartition_FWD_DEFINED__
typedef interface IFabricServiceGroupPartition IFabricServiceGroupPartition;

#endif 	/* __IFabricServiceGroupPartition_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext_FWD_DEFINED__
#define __IFabricCodePackageActivationContext_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext IFabricCodePackageActivationContext;

#endif 	/* __IFabricCodePackageActivationContext_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext2_FWD_DEFINED__
#define __IFabricCodePackageActivationContext2_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext2 IFabricCodePackageActivationContext2;

#endif 	/* __IFabricCodePackageActivationContext2_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext3_FWD_DEFINED__
#define __IFabricCodePackageActivationContext3_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext3 IFabricCodePackageActivationContext3;

#endif 	/* __IFabricCodePackageActivationContext3_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext4_FWD_DEFINED__
#define __IFabricCodePackageActivationContext4_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext4 IFabricCodePackageActivationContext4;

#endif 	/* __IFabricCodePackageActivationContext4_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext5_FWD_DEFINED__
#define __IFabricCodePackageActivationContext5_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext5 IFabricCodePackageActivationContext5;

#endif 	/* __IFabricCodePackageActivationContext5_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext6_FWD_DEFINED__
#define __IFabricCodePackageActivationContext6_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext6 IFabricCodePackageActivationContext6;

#endif 	/* __IFabricCodePackageActivationContext6_FWD_DEFINED__ */


#ifndef __IFabricCodePackage_FWD_DEFINED__
#define __IFabricCodePackage_FWD_DEFINED__
typedef interface IFabricCodePackage IFabricCodePackage;

#endif 	/* __IFabricCodePackage_FWD_DEFINED__ */


#ifndef __IFabricCodePackage2_FWD_DEFINED__
#define __IFabricCodePackage2_FWD_DEFINED__
typedef interface IFabricCodePackage2 IFabricCodePackage2;

#endif 	/* __IFabricCodePackage2_FWD_DEFINED__ */


#ifndef __IFabricConfigurationPackage_FWD_DEFINED__
#define __IFabricConfigurationPackage_FWD_DEFINED__
typedef interface IFabricConfigurationPackage IFabricConfigurationPackage;

#endif 	/* __IFabricConfigurationPackage_FWD_DEFINED__ */


#ifndef __IFabricConfigurationPackage2_FWD_DEFINED__
#define __IFabricConfigurationPackage2_FWD_DEFINED__
typedef interface IFabricConfigurationPackage2 IFabricConfigurationPackage2;

#endif 	/* __IFabricConfigurationPackage2_FWD_DEFINED__ */


#ifndef __IFabricDataPackage_FWD_DEFINED__
#define __IFabricDataPackage_FWD_DEFINED__
typedef interface IFabricDataPackage IFabricDataPackage;

#endif 	/* __IFabricDataPackage_FWD_DEFINED__ */


#ifndef __IFabricConfigurationPackageChangeHandler_FWD_DEFINED__
#define __IFabricConfigurationPackageChangeHandler_FWD_DEFINED__
typedef interface IFabricConfigurationPackageChangeHandler IFabricConfigurationPackageChangeHandler;

#endif 	/* __IFabricConfigurationPackageChangeHandler_FWD_DEFINED__ */


#ifndef __IFabricDataPackageChangeHandler_FWD_DEFINED__
#define __IFabricDataPackageChangeHandler_FWD_DEFINED__
typedef interface IFabricDataPackageChangeHandler IFabricDataPackageChangeHandler;

#endif 	/* __IFabricDataPackageChangeHandler_FWD_DEFINED__ */


#ifndef __IFabricTransactionBase_FWD_DEFINED__
#define __IFabricTransactionBase_FWD_DEFINED__
typedef interface IFabricTransactionBase IFabricTransactionBase;

#endif 	/* __IFabricTransactionBase_FWD_DEFINED__ */


#ifndef __IFabricTransaction_FWD_DEFINED__
#define __IFabricTransaction_FWD_DEFINED__
typedef interface IFabricTransaction IFabricTransaction;

#endif 	/* __IFabricTransaction_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica IFabricKeyValueStoreReplica;

#endif 	/* __IFabricKeyValueStoreReplica_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica2_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica2_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica2 IFabricKeyValueStoreReplica2;

#endif 	/* __IFabricKeyValueStoreReplica2_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica3_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica3_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica3 IFabricKeyValueStoreReplica3;

#endif 	/* __IFabricKeyValueStoreReplica3_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreItemEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemEnumerator IFabricKeyValueStoreItemEnumerator;

#endif 	/* __IFabricKeyValueStoreItemEnumerator_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreItemMetadataEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemMetadataEnumerator IFabricKeyValueStoreItemMetadataEnumerator;

#endif 	/* __IFabricKeyValueStoreItemMetadataEnumerator_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemResult_FWD_DEFINED__
#define __IFabricKeyValueStoreItemResult_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemResult IFabricKeyValueStoreItemResult;

#endif 	/* __IFabricKeyValueStoreItemResult_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataResult_FWD_DEFINED__
#define __IFabricKeyValueStoreItemMetadataResult_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemMetadataResult IFabricKeyValueStoreItemMetadataResult;

#endif 	/* __IFabricKeyValueStoreItemMetadataResult_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotification_FWD_DEFINED__
#define __IFabricKeyValueStoreNotification_FWD_DEFINED__
typedef interface IFabricKeyValueStoreNotification IFabricKeyValueStoreNotification;

#endif 	/* __IFabricKeyValueStoreNotification_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotificationEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreNotificationEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreNotificationEnumerator IFabricKeyValueStoreNotificationEnumerator;

#endif 	/* __IFabricKeyValueStoreNotificationEnumerator_FWD_DEFINED__ */


#ifndef __IFabricNodeContextResult_FWD_DEFINED__
#define __IFabricNodeContextResult_FWD_DEFINED__
typedef interface IFabricNodeContextResult IFabricNodeContextResult;

#endif 	/* __IFabricNodeContextResult_FWD_DEFINED__ */


#ifndef __IFabricNodeContextResult2_FWD_DEFINED__
#define __IFabricNodeContextResult2_FWD_DEFINED__
typedef interface IFabricNodeContextResult2 IFabricNodeContextResult2;

#endif 	/* __IFabricNodeContextResult2_FWD_DEFINED__ */


#ifndef __IFabricReplicatorSettingsResult_FWD_DEFINED__
#define __IFabricReplicatorSettingsResult_FWD_DEFINED__
typedef interface IFabricReplicatorSettingsResult IFabricReplicatorSettingsResult;

#endif 	/* __IFabricReplicatorSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricEseLocalStoreSettingsResult_FWD_DEFINED__
#define __IFabricEseLocalStoreSettingsResult_FWD_DEFINED__
typedef interface IFabricEseLocalStoreSettingsResult IFabricEseLocalStoreSettingsResult;

#endif 	/* __IFabricEseLocalStoreSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricSecurityCredentialsResult_FWD_DEFINED__
#define __IFabricSecurityCredentialsResult_FWD_DEFINED__
typedef interface IFabricSecurityCredentialsResult IFabricSecurityCredentialsResult;

#endif 	/* __IFabricSecurityCredentialsResult_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivator_FWD_DEFINED__
#define __IFabricCodePackageActivator_FWD_DEFINED__
typedef interface IFabricCodePackageActivator IFabricCodePackageActivator;

#endif 	/* __IFabricCodePackageActivator_FWD_DEFINED__ */


#ifndef __IFabricCodePackageEventHandler_FWD_DEFINED__
#define __IFabricCodePackageEventHandler_FWD_DEFINED__
typedef interface IFabricCodePackageEventHandler IFabricCodePackageEventHandler;

#endif 	/* __IFabricCodePackageEventHandler_FWD_DEFINED__ */


#ifndef __FabricRuntime_FWD_DEFINED__
#define __FabricRuntime_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricRuntime FabricRuntime;
#else
typedef struct FabricRuntime FabricRuntime;
#endif /* __cplusplus */

#endif 	/* __FabricRuntime_FWD_DEFINED__ */


#ifndef __IFabricRuntime_FWD_DEFINED__
#define __IFabricRuntime_FWD_DEFINED__
typedef interface IFabricRuntime IFabricRuntime;

#endif 	/* __IFabricRuntime_FWD_DEFINED__ */


#ifndef __IFabricStatelessServiceFactory_FWD_DEFINED__
#define __IFabricStatelessServiceFactory_FWD_DEFINED__
typedef interface IFabricStatelessServiceFactory IFabricStatelessServiceFactory;

#endif 	/* __IFabricStatelessServiceFactory_FWD_DEFINED__ */


#ifndef __IFabricStatelessServiceInstance_FWD_DEFINED__
#define __IFabricStatelessServiceInstance_FWD_DEFINED__
typedef interface IFabricStatelessServiceInstance IFabricStatelessServiceInstance;

#endif 	/* __IFabricStatelessServiceInstance_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition_FWD_DEFINED__
#define __IFabricStatelessServicePartition_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition IFabricStatelessServicePartition;

#endif 	/* __IFabricStatelessServicePartition_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition1_FWD_DEFINED__
#define __IFabricStatelessServicePartition1_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition1 IFabricStatelessServicePartition1;

#endif 	/* __IFabricStatelessServicePartition1_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition2_FWD_DEFINED__
#define __IFabricStatelessServicePartition2_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition2 IFabricStatelessServicePartition2;

#endif 	/* __IFabricStatelessServicePartition2_FWD_DEFINED__ */


#ifndef __IFabricStatelessServicePartition3_FWD_DEFINED__
#define __IFabricStatelessServicePartition3_FWD_DEFINED__
typedef interface IFabricStatelessServicePartition3 IFabricStatelessServicePartition3;

#endif 	/* __IFabricStatelessServicePartition3_FWD_DEFINED__ */


#ifndef __IFabricStatefulServiceFactory_FWD_DEFINED__
#define __IFabricStatefulServiceFactory_FWD_DEFINED__
typedef interface IFabricStatefulServiceFactory IFabricStatefulServiceFactory;

#endif 	/* __IFabricStatefulServiceFactory_FWD_DEFINED__ */


#ifndef __IFabricStatefulServiceReplica_FWD_DEFINED__
#define __IFabricStatefulServiceReplica_FWD_DEFINED__
typedef interface IFabricStatefulServiceReplica IFabricStatefulServiceReplica;

#endif 	/* __IFabricStatefulServiceReplica_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition_FWD_DEFINED__
#define __IFabricStatefulServicePartition_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition IFabricStatefulServicePartition;

#endif 	/* __IFabricStatefulServicePartition_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition1_FWD_DEFINED__
#define __IFabricStatefulServicePartition1_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition1 IFabricStatefulServicePartition1;

#endif 	/* __IFabricStatefulServicePartition1_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition2_FWD_DEFINED__
#define __IFabricStatefulServicePartition2_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition2 IFabricStatefulServicePartition2;

#endif 	/* __IFabricStatefulServicePartition2_FWD_DEFINED__ */


#ifndef __IFabricStatefulServicePartition3_FWD_DEFINED__
#define __IFabricStatefulServicePartition3_FWD_DEFINED__
typedef interface IFabricStatefulServicePartition3 IFabricStatefulServicePartition3;

#endif 	/* __IFabricStatefulServicePartition3_FWD_DEFINED__ */


#ifndef __IFabricStateReplicator_FWD_DEFINED__
#define __IFabricStateReplicator_FWD_DEFINED__
typedef interface IFabricStateReplicator IFabricStateReplicator;

#endif 	/* __IFabricStateReplicator_FWD_DEFINED__ */


#ifndef __IFabricStateReplicator2_FWD_DEFINED__
#define __IFabricStateReplicator2_FWD_DEFINED__
typedef interface IFabricStateReplicator2 IFabricStateReplicator2;

#endif 	/* __IFabricStateReplicator2_FWD_DEFINED__ */


#ifndef __IFabricStateProvider_FWD_DEFINED__
#define __IFabricStateProvider_FWD_DEFINED__
typedef interface IFabricStateProvider IFabricStateProvider;

#endif 	/* __IFabricStateProvider_FWD_DEFINED__ */


#ifndef __IFabricOperation_FWD_DEFINED__
#define __IFabricOperation_FWD_DEFINED__
typedef interface IFabricOperation IFabricOperation;

#endif 	/* __IFabricOperation_FWD_DEFINED__ */


#ifndef __IFabricOperationData_FWD_DEFINED__
#define __IFabricOperationData_FWD_DEFINED__
typedef interface IFabricOperationData IFabricOperationData;

#endif 	/* __IFabricOperationData_FWD_DEFINED__ */


#ifndef __IFabricOperationStream_FWD_DEFINED__
#define __IFabricOperationStream_FWD_DEFINED__
typedef interface IFabricOperationStream IFabricOperationStream;

#endif 	/* __IFabricOperationStream_FWD_DEFINED__ */


#ifndef __IFabricOperationStream2_FWD_DEFINED__
#define __IFabricOperationStream2_FWD_DEFINED__
typedef interface IFabricOperationStream2 IFabricOperationStream2;

#endif 	/* __IFabricOperationStream2_FWD_DEFINED__ */


#ifndef __IFabricOperationDataStream_FWD_DEFINED__
#define __IFabricOperationDataStream_FWD_DEFINED__
typedef interface IFabricOperationDataStream IFabricOperationDataStream;

#endif 	/* __IFabricOperationDataStream_FWD_DEFINED__ */


#ifndef __IFabricReplicator_FWD_DEFINED__
#define __IFabricReplicator_FWD_DEFINED__
typedef interface IFabricReplicator IFabricReplicator;

#endif 	/* __IFabricReplicator_FWD_DEFINED__ */


#ifndef __IFabricPrimaryReplicator_FWD_DEFINED__
#define __IFabricPrimaryReplicator_FWD_DEFINED__
typedef interface IFabricPrimaryReplicator IFabricPrimaryReplicator;

#endif 	/* __IFabricPrimaryReplicator_FWD_DEFINED__ */


#ifndef __IFabricReplicatorCatchupSpecificQuorum_FWD_DEFINED__
#define __IFabricReplicatorCatchupSpecificQuorum_FWD_DEFINED__
typedef interface IFabricReplicatorCatchupSpecificQuorum IFabricReplicatorCatchupSpecificQuorum;

#endif 	/* __IFabricReplicatorCatchupSpecificQuorum_FWD_DEFINED__ */


#ifndef __IFabricAtomicGroupStateReplicator_FWD_DEFINED__
#define __IFabricAtomicGroupStateReplicator_FWD_DEFINED__
typedef interface IFabricAtomicGroupStateReplicator IFabricAtomicGroupStateReplicator;

#endif 	/* __IFabricAtomicGroupStateReplicator_FWD_DEFINED__ */


#ifndef __IFabricAtomicGroupStateProvider_FWD_DEFINED__
#define __IFabricAtomicGroupStateProvider_FWD_DEFINED__
typedef interface IFabricAtomicGroupStateProvider IFabricAtomicGroupStateProvider;

#endif 	/* __IFabricAtomicGroupStateProvider_FWD_DEFINED__ */


#ifndef __IFabricServiceGroupFactory_FWD_DEFINED__
#define __IFabricServiceGroupFactory_FWD_DEFINED__
typedef interface IFabricServiceGroupFactory IFabricServiceGroupFactory;

#endif 	/* __IFabricServiceGroupFactory_FWD_DEFINED__ */


#ifndef __IFabricServiceGroupFactoryBuilder_FWD_DEFINED__
#define __IFabricServiceGroupFactoryBuilder_FWD_DEFINED__
typedef interface IFabricServiceGroupFactoryBuilder IFabricServiceGroupFactoryBuilder;

#endif 	/* __IFabricServiceGroupFactoryBuilder_FWD_DEFINED__ */


#ifndef __IFabricServiceGroupPartition_FWD_DEFINED__
#define __IFabricServiceGroupPartition_FWD_DEFINED__
typedef interface IFabricServiceGroupPartition IFabricServiceGroupPartition;

#endif 	/* __IFabricServiceGroupPartition_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext_FWD_DEFINED__
#define __IFabricCodePackageActivationContext_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext IFabricCodePackageActivationContext;

#endif 	/* __IFabricCodePackageActivationContext_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext2_FWD_DEFINED__
#define __IFabricCodePackageActivationContext2_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext2 IFabricCodePackageActivationContext2;

#endif 	/* __IFabricCodePackageActivationContext2_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext3_FWD_DEFINED__
#define __IFabricCodePackageActivationContext3_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext3 IFabricCodePackageActivationContext3;

#endif 	/* __IFabricCodePackageActivationContext3_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext4_FWD_DEFINED__
#define __IFabricCodePackageActivationContext4_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext4 IFabricCodePackageActivationContext4;

#endif 	/* __IFabricCodePackageActivationContext4_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext5_FWD_DEFINED__
#define __IFabricCodePackageActivationContext5_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext5 IFabricCodePackageActivationContext5;

#endif 	/* __IFabricCodePackageActivationContext5_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext6_FWD_DEFINED__
#define __IFabricCodePackageActivationContext6_FWD_DEFINED__
typedef interface IFabricCodePackageActivationContext6 IFabricCodePackageActivationContext6;

#endif 	/* __IFabricCodePackageActivationContext6_FWD_DEFINED__ */


#ifndef __IFabricCodePackage_FWD_DEFINED__
#define __IFabricCodePackage_FWD_DEFINED__
typedef interface IFabricCodePackage IFabricCodePackage;

#endif 	/* __IFabricCodePackage_FWD_DEFINED__ */


#ifndef __IFabricCodePackage2_FWD_DEFINED__
#define __IFabricCodePackage2_FWD_DEFINED__
typedef interface IFabricCodePackage2 IFabricCodePackage2;

#endif 	/* __IFabricCodePackage2_FWD_DEFINED__ */


#ifndef __IFabricConfigurationPackage_FWD_DEFINED__
#define __IFabricConfigurationPackage_FWD_DEFINED__
typedef interface IFabricConfigurationPackage IFabricConfigurationPackage;

#endif 	/* __IFabricConfigurationPackage_FWD_DEFINED__ */


#ifndef __IFabricConfigurationPackage2_FWD_DEFINED__
#define __IFabricConfigurationPackage2_FWD_DEFINED__
typedef interface IFabricConfigurationPackage2 IFabricConfigurationPackage2;

#endif 	/* __IFabricConfigurationPackage2_FWD_DEFINED__ */


#ifndef __IFabricDataPackage_FWD_DEFINED__
#define __IFabricDataPackage_FWD_DEFINED__
typedef interface IFabricDataPackage IFabricDataPackage;

#endif 	/* __IFabricDataPackage_FWD_DEFINED__ */


#ifndef __IFabricCodePackageChangeHandler_FWD_DEFINED__
#define __IFabricCodePackageChangeHandler_FWD_DEFINED__
typedef interface IFabricCodePackageChangeHandler IFabricCodePackageChangeHandler;

#endif 	/* __IFabricCodePackageChangeHandler_FWD_DEFINED__ */


#ifndef __IFabricConfigurationPackageChangeHandler_FWD_DEFINED__
#define __IFabricConfigurationPackageChangeHandler_FWD_DEFINED__
typedef interface IFabricConfigurationPackageChangeHandler IFabricConfigurationPackageChangeHandler;

#endif 	/* __IFabricConfigurationPackageChangeHandler_FWD_DEFINED__ */


#ifndef __IFabricDataPackageChangeHandler_FWD_DEFINED__
#define __IFabricDataPackageChangeHandler_FWD_DEFINED__
typedef interface IFabricDataPackageChangeHandler IFabricDataPackageChangeHandler;

#endif 	/* __IFabricDataPackageChangeHandler_FWD_DEFINED__ */


#ifndef __IFabricProcessExitHandler_FWD_DEFINED__
#define __IFabricProcessExitHandler_FWD_DEFINED__
typedef interface IFabricProcessExitHandler IFabricProcessExitHandler;

#endif 	/* __IFabricProcessExitHandler_FWD_DEFINED__ */


#ifndef __IFabricTransactionBase_FWD_DEFINED__
#define __IFabricTransactionBase_FWD_DEFINED__
typedef interface IFabricTransactionBase IFabricTransactionBase;

#endif 	/* __IFabricTransactionBase_FWD_DEFINED__ */


#ifndef __IFabricTransaction_FWD_DEFINED__
#define __IFabricTransaction_FWD_DEFINED__
typedef interface IFabricTransaction IFabricTransaction;

#endif 	/* __IFabricTransaction_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica IFabricKeyValueStoreReplica;

#endif 	/* __IFabricKeyValueStoreReplica_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica2_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica2_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica2 IFabricKeyValueStoreReplica2;

#endif 	/* __IFabricKeyValueStoreReplica2_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica3_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica3_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica3 IFabricKeyValueStoreReplica3;

#endif 	/* __IFabricKeyValueStoreReplica3_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica4_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica4_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica4 IFabricKeyValueStoreReplica4;

#endif 	/* __IFabricKeyValueStoreReplica4_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica5_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica5_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica5 IFabricKeyValueStoreReplica5;

#endif 	/* __IFabricKeyValueStoreReplica5_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica6_FWD_DEFINED__
#define __IFabricKeyValueStoreReplica6_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplica6 IFabricKeyValueStoreReplica6;

#endif 	/* __IFabricKeyValueStoreReplica6_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreEnumerator IFabricKeyValueStoreEnumerator;

#endif 	/* __IFabricKeyValueStoreEnumerator_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreEnumerator2_FWD_DEFINED__
#define __IFabricKeyValueStoreEnumerator2_FWD_DEFINED__
typedef interface IFabricKeyValueStoreEnumerator2 IFabricKeyValueStoreEnumerator2;

#endif 	/* __IFabricKeyValueStoreEnumerator2_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreItemEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemEnumerator IFabricKeyValueStoreItemEnumerator;

#endif 	/* __IFabricKeyValueStoreItemEnumerator_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreItemMetadataEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemMetadataEnumerator IFabricKeyValueStoreItemMetadataEnumerator;

#endif 	/* __IFabricKeyValueStoreItemMetadataEnumerator_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotificationEnumerator_FWD_DEFINED__
#define __IFabricKeyValueStoreNotificationEnumerator_FWD_DEFINED__
typedef interface IFabricKeyValueStoreNotificationEnumerator IFabricKeyValueStoreNotificationEnumerator;

#endif 	/* __IFabricKeyValueStoreNotificationEnumerator_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemEnumerator2_FWD_DEFINED__
#define __IFabricKeyValueStoreItemEnumerator2_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemEnumerator2 IFabricKeyValueStoreItemEnumerator2;

#endif 	/* __IFabricKeyValueStoreItemEnumerator2_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataEnumerator2_FWD_DEFINED__
#define __IFabricKeyValueStoreItemMetadataEnumerator2_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemMetadataEnumerator2 IFabricKeyValueStoreItemMetadataEnumerator2;

#endif 	/* __IFabricKeyValueStoreItemMetadataEnumerator2_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotificationEnumerator2_FWD_DEFINED__
#define __IFabricKeyValueStoreNotificationEnumerator2_FWD_DEFINED__
typedef interface IFabricKeyValueStoreNotificationEnumerator2 IFabricKeyValueStoreNotificationEnumerator2;

#endif 	/* __IFabricKeyValueStoreNotificationEnumerator2_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemResult_FWD_DEFINED__
#define __IFabricKeyValueStoreItemResult_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemResult IFabricKeyValueStoreItemResult;

#endif 	/* __IFabricKeyValueStoreItemResult_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataResult_FWD_DEFINED__
#define __IFabricKeyValueStoreItemMetadataResult_FWD_DEFINED__
typedef interface IFabricKeyValueStoreItemMetadataResult IFabricKeyValueStoreItemMetadataResult;

#endif 	/* __IFabricKeyValueStoreItemMetadataResult_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotification_FWD_DEFINED__
#define __IFabricKeyValueStoreNotification_FWD_DEFINED__
typedef interface IFabricKeyValueStoreNotification IFabricKeyValueStoreNotification;

#endif 	/* __IFabricKeyValueStoreNotification_FWD_DEFINED__ */


#ifndef __IFabricStoreEventHandler_FWD_DEFINED__
#define __IFabricStoreEventHandler_FWD_DEFINED__
typedef interface IFabricStoreEventHandler IFabricStoreEventHandler;

#endif 	/* __IFabricStoreEventHandler_FWD_DEFINED__ */


#ifndef __IFabricStoreEventHandler2_FWD_DEFINED__
#define __IFabricStoreEventHandler2_FWD_DEFINED__
typedef interface IFabricStoreEventHandler2 IFabricStoreEventHandler2;

#endif 	/* __IFabricStoreEventHandler2_FWD_DEFINED__ */


#ifndef __IFabricStorePostBackupHandler_FWD_DEFINED__
#define __IFabricStorePostBackupHandler_FWD_DEFINED__
typedef interface IFabricStorePostBackupHandler IFabricStorePostBackupHandler;

#endif 	/* __IFabricStorePostBackupHandler_FWD_DEFINED__ */


#ifndef __IFabricSecondaryEventHandler_FWD_DEFINED__
#define __IFabricSecondaryEventHandler_FWD_DEFINED__
typedef interface IFabricSecondaryEventHandler IFabricSecondaryEventHandler;

#endif 	/* __IFabricSecondaryEventHandler_FWD_DEFINED__ */


#ifndef __IFabricNodeContextResult_FWD_DEFINED__
#define __IFabricNodeContextResult_FWD_DEFINED__
typedef interface IFabricNodeContextResult IFabricNodeContextResult;

#endif 	/* __IFabricNodeContextResult_FWD_DEFINED__ */


#ifndef __IFabricNodeContextResult2_FWD_DEFINED__
#define __IFabricNodeContextResult2_FWD_DEFINED__
typedef interface IFabricNodeContextResult2 IFabricNodeContextResult2;

#endif 	/* __IFabricNodeContextResult2_FWD_DEFINED__ */


#ifndef __IFabricReplicatorSettingsResult_FWD_DEFINED__
#define __IFabricReplicatorSettingsResult_FWD_DEFINED__
typedef interface IFabricReplicatorSettingsResult IFabricReplicatorSettingsResult;

#endif 	/* __IFabricReplicatorSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricEseLocalStoreSettingsResult_FWD_DEFINED__
#define __IFabricEseLocalStoreSettingsResult_FWD_DEFINED__
typedef interface IFabricEseLocalStoreSettingsResult IFabricEseLocalStoreSettingsResult;

#endif 	/* __IFabricEseLocalStoreSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricSecurityCredentialsResult_FWD_DEFINED__
#define __IFabricSecurityCredentialsResult_FWD_DEFINED__
typedef interface IFabricSecurityCredentialsResult IFabricSecurityCredentialsResult;

#endif 	/* __IFabricSecurityCredentialsResult_FWD_DEFINED__ */


#ifndef __IFabricCodePackageActivator_FWD_DEFINED__
#define __IFabricCodePackageActivator_FWD_DEFINED__
typedef interface IFabricCodePackageActivator IFabricCodePackageActivator;

#endif 	/* __IFabricCodePackageActivator_FWD_DEFINED__ */


#ifndef __IFabricCodePackageEventHandler_FWD_DEFINED__
#define __IFabricCodePackageEventHandler_FWD_DEFINED__
typedef interface IFabricCodePackageEventHandler IFabricCodePackageEventHandler;

#endif 	/* __IFabricCodePackageEventHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricTypes.h"
#include "FabricCommon.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricruntime_0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif











































































extern RPC_IF_HANDLE __MIDL_itf_fabricruntime_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricruntime_0000_0000_v0_0_s_ifspec;


#ifndef __FabricRuntimeLib_LIBRARY_DEFINED__
#define __FabricRuntimeLib_LIBRARY_DEFINED__

/* library FabricRuntimeLib */
/* [version][helpstring][uuid] */ 


#pragma pack(push, 8)






























































#pragma pack(pop)

EXTERN_C const IID LIBID_FabricRuntimeLib;

#ifndef __IFabricRuntime_INTERFACE_DEFINED__
#define __IFabricRuntime_INTERFACE_DEFINED__

/* interface IFabricRuntime */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricRuntime;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cc53af8e-74cd-11df-ac3e-0024811e3892")
    IFabricRuntime : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRegisterStatelessServiceFactory( 
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatelessServiceFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRegisterStatelessServiceFactory( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterStatelessServiceFactory( 
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatelessServiceFactory *factory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRegisterStatefulServiceFactory( 
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatefulServiceFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRegisterStatefulServiceFactory( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterStatefulServiceFactory( 
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatefulServiceFactory *factory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateServiceGroupFactoryBuilder( 
            /* [retval][out] */ IFabricServiceGroupFactoryBuilder **builder) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRegisterServiceGroupFactory( 
            /* [in] */ LPCWSTR groupServiceType,
            /* [in] */ IFabricServiceGroupFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRegisterServiceGroupFactory( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterServiceGroupFactory( 
            /* [in] */ LPCWSTR groupServiceType,
            /* [in] */ IFabricServiceGroupFactory *factory) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricRuntimeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricRuntime * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricRuntime * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricRuntime * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRegisterStatelessServiceFactory )( 
            IFabricRuntime * This,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatelessServiceFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRegisterStatelessServiceFactory )( 
            IFabricRuntime * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStatelessServiceFactory )( 
            IFabricRuntime * This,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatelessServiceFactory *factory);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRegisterStatefulServiceFactory )( 
            IFabricRuntime * This,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatefulServiceFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRegisterStatefulServiceFactory )( 
            IFabricRuntime * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterStatefulServiceFactory )( 
            IFabricRuntime * This,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ IFabricStatefulServiceFactory *factory);
        
        HRESULT ( STDMETHODCALLTYPE *CreateServiceGroupFactoryBuilder )( 
            IFabricRuntime * This,
            /* [retval][out] */ IFabricServiceGroupFactoryBuilder **builder);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRegisterServiceGroupFactory )( 
            IFabricRuntime * This,
            /* [in] */ LPCWSTR groupServiceType,
            /* [in] */ IFabricServiceGroupFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRegisterServiceGroupFactory )( 
            IFabricRuntime * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterServiceGroupFactory )( 
            IFabricRuntime * This,
            /* [in] */ LPCWSTR groupServiceType,
            /* [in] */ IFabricServiceGroupFactory *factory);
        
        END_INTERFACE
    } IFabricRuntimeVtbl;

    interface IFabricRuntime
    {
        CONST_VTBL struct IFabricRuntimeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricRuntime_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricRuntime_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricRuntime_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricRuntime_BeginRegisterStatelessServiceFactory(This,serviceTypeName,factory,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRegisterStatelessServiceFactory(This,serviceTypeName,factory,timeoutMilliseconds,callback,context) ) 

#define IFabricRuntime_EndRegisterStatelessServiceFactory(This,context)	\
    ( (This)->lpVtbl -> EndRegisterStatelessServiceFactory(This,context) ) 

#define IFabricRuntime_RegisterStatelessServiceFactory(This,serviceTypeName,factory)	\
    ( (This)->lpVtbl -> RegisterStatelessServiceFactory(This,serviceTypeName,factory) ) 

#define IFabricRuntime_BeginRegisterStatefulServiceFactory(This,serviceTypeName,factory,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRegisterStatefulServiceFactory(This,serviceTypeName,factory,timeoutMilliseconds,callback,context) ) 

#define IFabricRuntime_EndRegisterStatefulServiceFactory(This,context)	\
    ( (This)->lpVtbl -> EndRegisterStatefulServiceFactory(This,context) ) 

#define IFabricRuntime_RegisterStatefulServiceFactory(This,serviceTypeName,factory)	\
    ( (This)->lpVtbl -> RegisterStatefulServiceFactory(This,serviceTypeName,factory) ) 

#define IFabricRuntime_CreateServiceGroupFactoryBuilder(This,builder)	\
    ( (This)->lpVtbl -> CreateServiceGroupFactoryBuilder(This,builder) ) 

#define IFabricRuntime_BeginRegisterServiceGroupFactory(This,groupServiceType,factory,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRegisterServiceGroupFactory(This,groupServiceType,factory,timeoutMilliseconds,callback,context) ) 

#define IFabricRuntime_EndRegisterServiceGroupFactory(This,context)	\
    ( (This)->lpVtbl -> EndRegisterServiceGroupFactory(This,context) ) 

#define IFabricRuntime_RegisterServiceGroupFactory(This,groupServiceType,factory)	\
    ( (This)->lpVtbl -> RegisterServiceGroupFactory(This,groupServiceType,factory) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricRuntime_INTERFACE_DEFINED__ */


#ifndef __IFabricProcessExitHandler_INTERFACE_DEFINED__
#define __IFabricProcessExitHandler_INTERFACE_DEFINED__

/* interface IFabricProcessExitHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricProcessExitHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c58d50a2-01f0-4267-bbe7-223b565c1346")
    IFabricProcessExitHandler : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE FabricProcessExited( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricProcessExitHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricProcessExitHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricProcessExitHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricProcessExitHandler * This);
        
        void ( STDMETHODCALLTYPE *FabricProcessExited )( 
            IFabricProcessExitHandler * This);
        
        END_INTERFACE
    } IFabricProcessExitHandlerVtbl;

    interface IFabricProcessExitHandler
    {
        CONST_VTBL struct IFabricProcessExitHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricProcessExitHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricProcessExitHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricProcessExitHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricProcessExitHandler_FabricProcessExited(This)	\
    ( (This)->lpVtbl -> FabricProcessExited(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricProcessExitHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricStatelessServiceFactory_INTERFACE_DEFINED__
#define __IFabricStatelessServiceFactory_INTERFACE_DEFINED__

/* interface IFabricStatelessServiceFactory */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatelessServiceFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cc53af8f-74cd-11df-ac3e-0024811e3892")
    IFabricStatelessServiceFactory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_INSTANCE_ID instanceId,
            /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatelessServiceFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatelessServiceFactory * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatelessServiceFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatelessServiceFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateInstance )( 
            IFabricStatelessServiceFactory * This,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_INSTANCE_ID instanceId,
            /* [retval][out] */ IFabricStatelessServiceInstance **serviceInstance);
        
        END_INTERFACE
    } IFabricStatelessServiceFactoryVtbl;

    interface IFabricStatelessServiceFactory
    {
        CONST_VTBL struct IFabricStatelessServiceFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatelessServiceFactory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatelessServiceFactory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatelessServiceFactory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatelessServiceFactory_CreateInstance(This,serviceTypeName,serviceName,initializationDataLength,initializationData,partitionId,instanceId,serviceInstance)	\
    ( (This)->lpVtbl -> CreateInstance(This,serviceTypeName,serviceName,initializationDataLength,initializationData,partitionId,instanceId,serviceInstance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatelessServiceFactory_INTERFACE_DEFINED__ */


#ifndef __IFabricStatelessServiceInstance_INTERFACE_DEFINED__
#define __IFabricStatelessServiceInstance_INTERFACE_DEFINED__

/* interface IFabricStatelessServiceInstance */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatelessServiceInstance;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cc53af90-74cd-11df-ac3e-0024811e3892")
    IFabricStatelessServiceInstance : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricStatelessServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual void STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatelessServiceInstanceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatelessServiceInstance * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatelessServiceInstance * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatelessServiceInstance * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricStatelessServiceInstance * This,
            /* [in] */ IFabricStatelessServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricStatelessServiceInstance * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricStatelessServiceInstance * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricStatelessServiceInstance * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricStatelessServiceInstance * This);
        
        END_INTERFACE
    } IFabricStatelessServiceInstanceVtbl;

    interface IFabricStatelessServiceInstance
    {
        CONST_VTBL struct IFabricStatelessServiceInstanceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatelessServiceInstance_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatelessServiceInstance_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatelessServiceInstance_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatelessServiceInstance_BeginOpen(This,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,partition,callback,context) ) 

#define IFabricStatelessServiceInstance_EndOpen(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndOpen(This,context,serviceAddress) ) 

#define IFabricStatelessServiceInstance_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricStatelessServiceInstance_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricStatelessServiceInstance_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatelessServiceInstance_INTERFACE_DEFINED__ */


#ifndef __IFabricStatelessServicePartition_INTERFACE_DEFINED__
#define __IFabricStatelessServicePartition_INTERFACE_DEFINED__

/* interface IFabricStatelessServicePartition */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatelessServicePartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cc53af91-74cd-11df-ac3e-0024811e3892")
    IFabricStatelessServicePartition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPartitionInfo( 
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportLoad( 
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportFault( 
            /* [in] */ FABRIC_FAULT_TYPE faultType) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatelessServicePartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatelessServicePartition * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatelessServicePartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatelessServicePartition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatelessServicePartition * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatelessServicePartition * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatelessServicePartition * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        END_INTERFACE
    } IFabricStatelessServicePartitionVtbl;

    interface IFabricStatelessServicePartition
    {
        CONST_VTBL struct IFabricStatelessServicePartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatelessServicePartition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatelessServicePartition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatelessServicePartition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatelessServicePartition_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatelessServicePartition_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatelessServicePartition_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatelessServicePartition_INTERFACE_DEFINED__ */


#ifndef __IFabricStatelessServicePartition1_INTERFACE_DEFINED__
#define __IFabricStatelessServicePartition1_INTERFACE_DEFINED__

/* interface IFabricStatelessServicePartition1 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatelessServicePartition1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bf6bb505-7bd0-4371-b6c0-cba319a5e50b")
    IFabricStatelessServicePartition1 : public IFabricStatelessServicePartition
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportMoveCost( 
            /* [in] */ FABRIC_MOVE_COST moveCost) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatelessServicePartition1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatelessServicePartition1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatelessServicePartition1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatelessServicePartition1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatelessServicePartition1 * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatelessServicePartition1 * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatelessServicePartition1 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        HRESULT ( STDMETHODCALLTYPE *ReportMoveCost )( 
            IFabricStatelessServicePartition1 * This,
            /* [in] */ FABRIC_MOVE_COST moveCost);
        
        END_INTERFACE
    } IFabricStatelessServicePartition1Vtbl;

    interface IFabricStatelessServicePartition1
    {
        CONST_VTBL struct IFabricStatelessServicePartition1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatelessServicePartition1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatelessServicePartition1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatelessServicePartition1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatelessServicePartition1_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatelessServicePartition1_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatelessServicePartition1_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 


#define IFabricStatelessServicePartition1_ReportMoveCost(This,moveCost)	\
    ( (This)->lpVtbl -> ReportMoveCost(This,moveCost) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatelessServicePartition1_INTERFACE_DEFINED__ */


#ifndef __IFabricStatelessServicePartition2_INTERFACE_DEFINED__
#define __IFabricStatelessServicePartition2_INTERFACE_DEFINED__

/* interface IFabricStatelessServicePartition2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatelessServicePartition2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9ff35b6c-9d97-4312-93ad-7f34cbdb4ca4")
    IFabricStatelessServicePartition2 : public IFabricStatelessServicePartition1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportInstanceHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportPartitionHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatelessServicePartition2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatelessServicePartition2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatelessServicePartition2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatelessServicePartition2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatelessServicePartition2 * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatelessServicePartition2 * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatelessServicePartition2 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        HRESULT ( STDMETHODCALLTYPE *ReportMoveCost )( 
            IFabricStatelessServicePartition2 * This,
            /* [in] */ FABRIC_MOVE_COST moveCost);
        
        HRESULT ( STDMETHODCALLTYPE *ReportInstanceHealth )( 
            IFabricStatelessServicePartition2 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportPartitionHealth )( 
            IFabricStatelessServicePartition2 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        END_INTERFACE
    } IFabricStatelessServicePartition2Vtbl;

    interface IFabricStatelessServicePartition2
    {
        CONST_VTBL struct IFabricStatelessServicePartition2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatelessServicePartition2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatelessServicePartition2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatelessServicePartition2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatelessServicePartition2_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatelessServicePartition2_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatelessServicePartition2_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 


#define IFabricStatelessServicePartition2_ReportMoveCost(This,moveCost)	\
    ( (This)->lpVtbl -> ReportMoveCost(This,moveCost) ) 


#define IFabricStatelessServicePartition2_ReportInstanceHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportInstanceHealth(This,healthInfo) ) 

#define IFabricStatelessServicePartition2_ReportPartitionHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportPartitionHealth(This,healthInfo) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatelessServicePartition2_INTERFACE_DEFINED__ */


#ifndef __IFabricStatelessServicePartition3_INTERFACE_DEFINED__
#define __IFabricStatelessServicePartition3_INTERFACE_DEFINED__

/* interface IFabricStatelessServicePartition3 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatelessServicePartition3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2fa2000-70a7-4ed5-9d3e-0b7deca2433f")
    IFabricStatelessServicePartition3 : public IFabricStatelessServicePartition2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportInstanceHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportPartitionHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatelessServicePartition3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatelessServicePartition3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatelessServicePartition3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatelessServicePartition3 * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        HRESULT ( STDMETHODCALLTYPE *ReportMoveCost )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ FABRIC_MOVE_COST moveCost);
        
        HRESULT ( STDMETHODCALLTYPE *ReportInstanceHealth )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportPartitionHealth )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportInstanceHealth2 )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportPartitionHealth2 )( 
            IFabricStatelessServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        END_INTERFACE
    } IFabricStatelessServicePartition3Vtbl;

    interface IFabricStatelessServicePartition3
    {
        CONST_VTBL struct IFabricStatelessServicePartition3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatelessServicePartition3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatelessServicePartition3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatelessServicePartition3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatelessServicePartition3_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatelessServicePartition3_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatelessServicePartition3_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 


#define IFabricStatelessServicePartition3_ReportMoveCost(This,moveCost)	\
    ( (This)->lpVtbl -> ReportMoveCost(This,moveCost) ) 


#define IFabricStatelessServicePartition3_ReportInstanceHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportInstanceHealth(This,healthInfo) ) 

#define IFabricStatelessServicePartition3_ReportPartitionHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportPartitionHealth(This,healthInfo) ) 


#define IFabricStatelessServicePartition3_ReportInstanceHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportInstanceHealth2(This,healthInfo,sendOptions) ) 

#define IFabricStatelessServicePartition3_ReportPartitionHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportPartitionHealth2(This,healthInfo,sendOptions) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatelessServicePartition3_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServiceFactory_INTERFACE_DEFINED__
#define __IFabricStatefulServiceFactory_INTERFACE_DEFINED__

/* interface IFabricStatefulServiceFactory */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServiceFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77ff0c6b-6780-48ec-b4b0-61989327b0f2")
    IFabricStatefulServiceFactory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateReplica( 
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [retval][out] */ IFabricStatefulServiceReplica **serviceReplica) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServiceFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServiceFactory * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServiceFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServiceFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateReplica )( 
            IFabricStatefulServiceFactory * This,
            /* [in] */ LPCWSTR serviceTypeName,
            /* [in] */ FABRIC_URI serviceName,
            /* [in] */ ULONG initializationDataLength,
            /* [size_is][in] */ const byte *initializationData,
            /* [in] */ FABRIC_PARTITION_ID partitionId,
            /* [in] */ FABRIC_REPLICA_ID replicaId,
            /* [retval][out] */ IFabricStatefulServiceReplica **serviceReplica);
        
        END_INTERFACE
    } IFabricStatefulServiceFactoryVtbl;

    interface IFabricStatefulServiceFactory
    {
        CONST_VTBL struct IFabricStatefulServiceFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServiceFactory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServiceFactory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServiceFactory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServiceFactory_CreateReplica(This,serviceTypeName,serviceName,initializationDataLength,initializationData,partitionId,replicaId,serviceReplica)	\
    ( (This)->lpVtbl -> CreateReplica(This,serviceTypeName,serviceName,initializationDataLength,initializationData,partitionId,replicaId,serviceReplica) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServiceFactory_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServiceReplica_INTERFACE_DEFINED__
#define __IFabricStatefulServiceReplica_INTERFACE_DEFINED__

/* interface IFabricStatefulServiceReplica */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServiceReplica;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8ae3be0e-505d-4dc1-ad8f-0cb0f9576b8a")
    IFabricStatefulServiceReplica : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual void STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServiceReplicaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServiceReplica * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServiceReplica * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricStatefulServiceReplica * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricStatefulServiceReplica * This);
        
        END_INTERFACE
    } IFabricStatefulServiceReplicaVtbl;

    interface IFabricStatefulServiceReplica
    {
        CONST_VTBL struct IFabricStatefulServiceReplicaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServiceReplica_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServiceReplica_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServiceReplica_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServiceReplica_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricStatefulServiceReplica_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricStatefulServiceReplica_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricStatefulServiceReplica_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricStatefulServiceReplica_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricStatefulServiceReplica_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricStatefulServiceReplica_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServiceReplica_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServicePartition_INTERFACE_DEFINED__
#define __IFabricStatefulServicePartition_INTERFACE_DEFINED__

/* interface IFabricStatefulServicePartition */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServicePartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5beccc37-8655-4f20-bd43-f50691d7cd16")
    IFabricStatefulServicePartition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPartitionInfo( 
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReadStatus( 
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriteStatus( 
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateReplicator( 
            /* [in] */ IFabricStateProvider *stateProvider,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [out] */ IFabricReplicator **replicator,
            /* [retval][out] */ IFabricStateReplicator **stateReplicator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportLoad( 
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportFault( 
            /* [in] */ FABRIC_FAULT_TYPE faultType) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServicePartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServicePartition * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServicePartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServicePartition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatefulServicePartition * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetReadStatus )( 
            IFabricStatefulServicePartition * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteStatus )( 
            IFabricStatefulServicePartition * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
        
        HRESULT ( STDMETHODCALLTYPE *CreateReplicator )( 
            IFabricStatefulServicePartition * This,
            /* [in] */ IFabricStateProvider *stateProvider,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [out] */ IFabricReplicator **replicator,
            /* [retval][out] */ IFabricStateReplicator **stateReplicator);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatefulServicePartition * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatefulServicePartition * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        END_INTERFACE
    } IFabricStatefulServicePartitionVtbl;

    interface IFabricStatefulServicePartition
    {
        CONST_VTBL struct IFabricStatefulServicePartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServicePartition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServicePartition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServicePartition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServicePartition_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatefulServicePartition_GetReadStatus(This,readStatus)	\
    ( (This)->lpVtbl -> GetReadStatus(This,readStatus) ) 

#define IFabricStatefulServicePartition_GetWriteStatus(This,writeStatus)	\
    ( (This)->lpVtbl -> GetWriteStatus(This,writeStatus) ) 

#define IFabricStatefulServicePartition_CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator)	\
    ( (This)->lpVtbl -> CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator) ) 

#define IFabricStatefulServicePartition_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatefulServicePartition_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServicePartition_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServicePartition1_INTERFACE_DEFINED__
#define __IFabricStatefulServicePartition1_INTERFACE_DEFINED__

/* interface IFabricStatefulServicePartition1 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServicePartition1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c9c66f2f-9dff-4c87-bbe4-a08b4c4074cf")
    IFabricStatefulServicePartition1 : public IFabricStatefulServicePartition
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportMoveCost( 
            /* [in] */ FABRIC_MOVE_COST moveCost) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServicePartition1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServicePartition1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServicePartition1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServicePartition1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatefulServicePartition1 * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetReadStatus )( 
            IFabricStatefulServicePartition1 * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteStatus )( 
            IFabricStatefulServicePartition1 * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
        
        HRESULT ( STDMETHODCALLTYPE *CreateReplicator )( 
            IFabricStatefulServicePartition1 * This,
            /* [in] */ IFabricStateProvider *stateProvider,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [out] */ IFabricReplicator **replicator,
            /* [retval][out] */ IFabricStateReplicator **stateReplicator);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatefulServicePartition1 * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatefulServicePartition1 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        HRESULT ( STDMETHODCALLTYPE *ReportMoveCost )( 
            IFabricStatefulServicePartition1 * This,
            /* [in] */ FABRIC_MOVE_COST moveCost);
        
        END_INTERFACE
    } IFabricStatefulServicePartition1Vtbl;

    interface IFabricStatefulServicePartition1
    {
        CONST_VTBL struct IFabricStatefulServicePartition1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServicePartition1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServicePartition1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServicePartition1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServicePartition1_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatefulServicePartition1_GetReadStatus(This,readStatus)	\
    ( (This)->lpVtbl -> GetReadStatus(This,readStatus) ) 

#define IFabricStatefulServicePartition1_GetWriteStatus(This,writeStatus)	\
    ( (This)->lpVtbl -> GetWriteStatus(This,writeStatus) ) 

#define IFabricStatefulServicePartition1_CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator)	\
    ( (This)->lpVtbl -> CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator) ) 

#define IFabricStatefulServicePartition1_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatefulServicePartition1_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 


#define IFabricStatefulServicePartition1_ReportMoveCost(This,moveCost)	\
    ( (This)->lpVtbl -> ReportMoveCost(This,moveCost) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServicePartition1_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServicePartition2_INTERFACE_DEFINED__
#define __IFabricStatefulServicePartition2_INTERFACE_DEFINED__

/* interface IFabricStatefulServicePartition2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServicePartition2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("df27b476-fa25-459f-a7d3-87d3eec9c73c")
    IFabricStatefulServicePartition2 : public IFabricStatefulServicePartition1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportReplicaHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportPartitionHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServicePartition2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServicePartition2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServicePartition2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatefulServicePartition2 * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetReadStatus )( 
            IFabricStatefulServicePartition2 * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteStatus )( 
            IFabricStatefulServicePartition2 * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
        
        HRESULT ( STDMETHODCALLTYPE *CreateReplicator )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ IFabricStateProvider *stateProvider,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [out] */ IFabricReplicator **replicator,
            /* [retval][out] */ IFabricStateReplicator **stateReplicator);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        HRESULT ( STDMETHODCALLTYPE *ReportMoveCost )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ FABRIC_MOVE_COST moveCost);
        
        HRESULT ( STDMETHODCALLTYPE *ReportReplicaHealth )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportPartitionHealth )( 
            IFabricStatefulServicePartition2 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        END_INTERFACE
    } IFabricStatefulServicePartition2Vtbl;

    interface IFabricStatefulServicePartition2
    {
        CONST_VTBL struct IFabricStatefulServicePartition2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServicePartition2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServicePartition2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServicePartition2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServicePartition2_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatefulServicePartition2_GetReadStatus(This,readStatus)	\
    ( (This)->lpVtbl -> GetReadStatus(This,readStatus) ) 

#define IFabricStatefulServicePartition2_GetWriteStatus(This,writeStatus)	\
    ( (This)->lpVtbl -> GetWriteStatus(This,writeStatus) ) 

#define IFabricStatefulServicePartition2_CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator)	\
    ( (This)->lpVtbl -> CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator) ) 

#define IFabricStatefulServicePartition2_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatefulServicePartition2_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 


#define IFabricStatefulServicePartition2_ReportMoveCost(This,moveCost)	\
    ( (This)->lpVtbl -> ReportMoveCost(This,moveCost) ) 


#define IFabricStatefulServicePartition2_ReportReplicaHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportReplicaHealth(This,healthInfo) ) 

#define IFabricStatefulServicePartition2_ReportPartitionHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportPartitionHealth(This,healthInfo) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServicePartition2_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServicePartition3_INTERFACE_DEFINED__
#define __IFabricStatefulServicePartition3_INTERFACE_DEFINED__

/* interface IFabricStatefulServicePartition3 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServicePartition3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51f1269d-b061-4c1c-96cf-6508cece813b")
    IFabricStatefulServicePartition3 : public IFabricStatefulServicePartition2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportReplicaHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportPartitionHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServicePartition3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServicePartition3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServicePartition3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionInfo )( 
            IFabricStatefulServicePartition3 * This,
            /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetReadStatus )( 
            IFabricStatefulServicePartition3 * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteStatus )( 
            IFabricStatefulServicePartition3 * This,
            /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
        
        HRESULT ( STDMETHODCALLTYPE *CreateReplicator )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ IFabricStateProvider *stateProvider,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [out] */ IFabricReplicator **replicator,
            /* [retval][out] */ IFabricStateReplicator **stateReplicator);
        
        HRESULT ( STDMETHODCALLTYPE *ReportLoad )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ ULONG metricCount,
            /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        HRESULT ( STDMETHODCALLTYPE *ReportMoveCost )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ FABRIC_MOVE_COST moveCost);
        
        HRESULT ( STDMETHODCALLTYPE *ReportReplicaHealth )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportPartitionHealth )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportReplicaHealth2 )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportPartitionHealth2 )( 
            IFabricStatefulServicePartition3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        END_INTERFACE
    } IFabricStatefulServicePartition3Vtbl;

    interface IFabricStatefulServicePartition3
    {
        CONST_VTBL struct IFabricStatefulServicePartition3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServicePartition3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServicePartition3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServicePartition3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServicePartition3_GetPartitionInfo(This,bufferedValue)	\
    ( (This)->lpVtbl -> GetPartitionInfo(This,bufferedValue) ) 

#define IFabricStatefulServicePartition3_GetReadStatus(This,readStatus)	\
    ( (This)->lpVtbl -> GetReadStatus(This,readStatus) ) 

#define IFabricStatefulServicePartition3_GetWriteStatus(This,writeStatus)	\
    ( (This)->lpVtbl -> GetWriteStatus(This,writeStatus) ) 

#define IFabricStatefulServicePartition3_CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator)	\
    ( (This)->lpVtbl -> CreateReplicator(This,stateProvider,replicatorSettings,replicator,stateReplicator) ) 

#define IFabricStatefulServicePartition3_ReportLoad(This,metricCount,metrics)	\
    ( (This)->lpVtbl -> ReportLoad(This,metricCount,metrics) ) 

#define IFabricStatefulServicePartition3_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 


#define IFabricStatefulServicePartition3_ReportMoveCost(This,moveCost)	\
    ( (This)->lpVtbl -> ReportMoveCost(This,moveCost) ) 


#define IFabricStatefulServicePartition3_ReportReplicaHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportReplicaHealth(This,healthInfo) ) 

#define IFabricStatefulServicePartition3_ReportPartitionHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportPartitionHealth(This,healthInfo) ) 


#define IFabricStatefulServicePartition3_ReportReplicaHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportReplicaHealth2(This,healthInfo,sendOptions) ) 

#define IFabricStatefulServicePartition3_ReportPartitionHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportPartitionHealth2(This,healthInfo,sendOptions) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServicePartition3_INTERFACE_DEFINED__ */


#ifndef __IFabricStateProvider_INTERFACE_DEFINED__
#define __IFabricStateProvider_INTERFACE_DEFINED__

/* interface IFabricStateProvider */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStateProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3ebfec79-bd27-43f3-8be8-da38ee723951")
    IFabricStateProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber( 
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCopyContext( 
            /* [retval][out] */ IFabricOperationDataStream **copyContextStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCopyState( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            /* [in] */ IFabricOperationDataStream *copyContextStream,
            /* [retval][out] */ IFabricOperationDataStream **copyStateStream) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStateProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStateProvider * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStateProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStateProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdateEpoch )( 
            IFabricStateProvider * This,
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdateEpoch )( 
            IFabricStateProvider * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastCommittedSequenceNumber )( 
            IFabricStateProvider * This,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOnDataLoss )( 
            IFabricStateProvider * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOnDataLoss )( 
            IFabricStateProvider * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged);
        
        HRESULT ( STDMETHODCALLTYPE *GetCopyContext )( 
            IFabricStateProvider * This,
            /* [retval][out] */ IFabricOperationDataStream **copyContextStream);
        
        HRESULT ( STDMETHODCALLTYPE *GetCopyState )( 
            IFabricStateProvider * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            /* [in] */ IFabricOperationDataStream *copyContextStream,
            /* [retval][out] */ IFabricOperationDataStream **copyStateStream);
        
        END_INTERFACE
    } IFabricStateProviderVtbl;

    interface IFabricStateProvider
    {
        CONST_VTBL struct IFabricStateProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStateProvider_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStateProvider_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStateProvider_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStateProvider_BeginUpdateEpoch(This,epoch,previousEpochLastSequenceNumber,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdateEpoch(This,epoch,previousEpochLastSequenceNumber,callback,context) ) 

#define IFabricStateProvider_EndUpdateEpoch(This,context)	\
    ( (This)->lpVtbl -> EndUpdateEpoch(This,context) ) 

#define IFabricStateProvider_GetLastCommittedSequenceNumber(This,sequenceNumber)	\
    ( (This)->lpVtbl -> GetLastCommittedSequenceNumber(This,sequenceNumber) ) 

#define IFabricStateProvider_BeginOnDataLoss(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOnDataLoss(This,callback,context) ) 

#define IFabricStateProvider_EndOnDataLoss(This,context,isStateChanged)	\
    ( (This)->lpVtbl -> EndOnDataLoss(This,context,isStateChanged) ) 

#define IFabricStateProvider_GetCopyContext(This,copyContextStream)	\
    ( (This)->lpVtbl -> GetCopyContext(This,copyContextStream) ) 

#define IFabricStateProvider_GetCopyState(This,uptoSequenceNumber,copyContextStream,copyStateStream)	\
    ( (This)->lpVtbl -> GetCopyState(This,uptoSequenceNumber,copyContextStream,copyStateStream) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStateProvider_INTERFACE_DEFINED__ */


#ifndef __IFabricStateReplicator_INTERFACE_DEFINED__
#define __IFabricStateReplicator_INTERFACE_DEFINED__

/* interface IFabricStateReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStateReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89e9a978-c771-44f2-92e8-3bf271cabe9c")
    IFabricStateReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginReplicate( 
            /* [in] */ IFabricOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReplicate( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReplicationStream( 
            /* [retval][out] */ IFabricOperationStream **stream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCopyStream( 
            /* [retval][out] */ IFabricOperationStream **stream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings( 
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStateReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStateReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStateReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStateReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicate )( 
            IFabricStateReplicator * This,
            /* [in] */ IFabricOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReplicate )( 
            IFabricStateReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetReplicationStream )( 
            IFabricStateReplicator * This,
            /* [retval][out] */ IFabricOperationStream **stream);
        
        HRESULT ( STDMETHODCALLTYPE *GetCopyStream )( 
            IFabricStateReplicator * This,
            /* [retval][out] */ IFabricOperationStream **stream);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricStateReplicator * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        END_INTERFACE
    } IFabricStateReplicatorVtbl;

    interface IFabricStateReplicator
    {
        CONST_VTBL struct IFabricStateReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStateReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStateReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStateReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStateReplicator_BeginReplicate(This,operationData,callback,sequenceNumber,context)	\
    ( (This)->lpVtbl -> BeginReplicate(This,operationData,callback,sequenceNumber,context) ) 

#define IFabricStateReplicator_EndReplicate(This,context,sequenceNumber)	\
    ( (This)->lpVtbl -> EndReplicate(This,context,sequenceNumber) ) 

#define IFabricStateReplicator_GetReplicationStream(This,stream)	\
    ( (This)->lpVtbl -> GetReplicationStream(This,stream) ) 

#define IFabricStateReplicator_GetCopyStream(This,stream)	\
    ( (This)->lpVtbl -> GetCopyStream(This,stream) ) 

#define IFabricStateReplicator_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStateReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricReplicator_INTERFACE_DEFINED__
#define __IFabricReplicator_INTERFACE_DEFINED__

/* interface IFabricReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("067f144a-e5be-4f5e-a181-8b5593e20242")
    IFabricReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **replicationAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ FABRIC_REPLICA_ROLE role,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual void STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentProgress( 
            /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCatchUpCapability( 
            /* [out] */ FABRIC_SEQUENCE_NUMBER *fromSequenceNumber) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricReplicator * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **replicationAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricReplicator * This,
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ FABRIC_REPLICA_ROLE role,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdateEpoch )( 
            IFabricReplicator * This,
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdateEpoch )( 
            IFabricReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricReplicator * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentProgress )( 
            IFabricReplicator * This,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetCatchUpCapability )( 
            IFabricReplicator * This,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *fromSequenceNumber);
        
        END_INTERFACE
    } IFabricReplicatorVtbl;

    interface IFabricReplicator
    {
        CONST_VTBL struct IFabricReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricReplicator_BeginOpen(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,callback,context) ) 

#define IFabricReplicator_EndOpen(This,context,replicationAddress)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicationAddress) ) 

#define IFabricReplicator_BeginChangeRole(This,epoch,role,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,epoch,role,callback,context) ) 

#define IFabricReplicator_EndChangeRole(This,context)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context) ) 

#define IFabricReplicator_BeginUpdateEpoch(This,epoch,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdateEpoch(This,epoch,callback,context) ) 

#define IFabricReplicator_EndUpdateEpoch(This,context)	\
    ( (This)->lpVtbl -> EndUpdateEpoch(This,context) ) 

#define IFabricReplicator_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricReplicator_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricReplicator_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define IFabricReplicator_GetCurrentProgress(This,lastSequenceNumber)	\
    ( (This)->lpVtbl -> GetCurrentProgress(This,lastSequenceNumber) ) 

#define IFabricReplicator_GetCatchUpCapability(This,fromSequenceNumber)	\
    ( (This)->lpVtbl -> GetCatchUpCapability(This,fromSequenceNumber) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricPrimaryReplicator_INTERFACE_DEFINED__
#define __IFabricPrimaryReplicator_INTERFACE_DEFINED__

/* interface IFabricPrimaryReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricPrimaryReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("564e50dd-c3a4-4600-a60e-6658874307ae")
    IFabricPrimaryReplicator : public IFabricReplicator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateCatchUpReplicaSetConfiguration( 
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration,
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *previousConfiguration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginWaitForCatchUpQuorum( 
            /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndWaitForCatchUpQuorum( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateCurrentReplicaSetConfiguration( 
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginBuildReplica( 
            /* [in] */ const FABRIC_REPLICA_INFORMATION *replica,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndBuildReplica( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveReplica( 
            /* [in] */ FABRIC_REPLICA_ID replicaId) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricPrimaryReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricPrimaryReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricPrimaryReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **replicationAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ FABRIC_REPLICA_ROLE role,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdateEpoch )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdateEpoch )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricPrimaryReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentProgress )( 
            IFabricPrimaryReplicator * This,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetCatchUpCapability )( 
            IFabricPrimaryReplicator * This,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *fromSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOnDataLoss )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOnDataLoss )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateCatchUpReplicaSetConfiguration )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration,
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *previousConfiguration);
        
        HRESULT ( STDMETHODCALLTYPE *BeginWaitForCatchUpQuorum )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndWaitForCatchUpQuorum )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateCurrentReplicaSetConfiguration )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration);
        
        HRESULT ( STDMETHODCALLTYPE *BeginBuildReplica )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ const FABRIC_REPLICA_INFORMATION *replica,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndBuildReplica )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveReplica )( 
            IFabricPrimaryReplicator * This,
            /* [in] */ FABRIC_REPLICA_ID replicaId);
        
        END_INTERFACE
    } IFabricPrimaryReplicatorVtbl;

    interface IFabricPrimaryReplicator
    {
        CONST_VTBL struct IFabricPrimaryReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricPrimaryReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricPrimaryReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricPrimaryReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricPrimaryReplicator_BeginOpen(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,callback,context) ) 

#define IFabricPrimaryReplicator_EndOpen(This,context,replicationAddress)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicationAddress) ) 

#define IFabricPrimaryReplicator_BeginChangeRole(This,epoch,role,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,epoch,role,callback,context) ) 

#define IFabricPrimaryReplicator_EndChangeRole(This,context)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context) ) 

#define IFabricPrimaryReplicator_BeginUpdateEpoch(This,epoch,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdateEpoch(This,epoch,callback,context) ) 

#define IFabricPrimaryReplicator_EndUpdateEpoch(This,context)	\
    ( (This)->lpVtbl -> EndUpdateEpoch(This,context) ) 

#define IFabricPrimaryReplicator_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricPrimaryReplicator_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricPrimaryReplicator_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define IFabricPrimaryReplicator_GetCurrentProgress(This,lastSequenceNumber)	\
    ( (This)->lpVtbl -> GetCurrentProgress(This,lastSequenceNumber) ) 

#define IFabricPrimaryReplicator_GetCatchUpCapability(This,fromSequenceNumber)	\
    ( (This)->lpVtbl -> GetCatchUpCapability(This,fromSequenceNumber) ) 


#define IFabricPrimaryReplicator_BeginOnDataLoss(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOnDataLoss(This,callback,context) ) 

#define IFabricPrimaryReplicator_EndOnDataLoss(This,context,isStateChanged)	\
    ( (This)->lpVtbl -> EndOnDataLoss(This,context,isStateChanged) ) 

#define IFabricPrimaryReplicator_UpdateCatchUpReplicaSetConfiguration(This,currentConfiguration,previousConfiguration)	\
    ( (This)->lpVtbl -> UpdateCatchUpReplicaSetConfiguration(This,currentConfiguration,previousConfiguration) ) 

#define IFabricPrimaryReplicator_BeginWaitForCatchUpQuorum(This,catchUpMode,callback,context)	\
    ( (This)->lpVtbl -> BeginWaitForCatchUpQuorum(This,catchUpMode,callback,context) ) 

#define IFabricPrimaryReplicator_EndWaitForCatchUpQuorum(This,context)	\
    ( (This)->lpVtbl -> EndWaitForCatchUpQuorum(This,context) ) 

#define IFabricPrimaryReplicator_UpdateCurrentReplicaSetConfiguration(This,currentConfiguration)	\
    ( (This)->lpVtbl -> UpdateCurrentReplicaSetConfiguration(This,currentConfiguration) ) 

#define IFabricPrimaryReplicator_BeginBuildReplica(This,replica,callback,context)	\
    ( (This)->lpVtbl -> BeginBuildReplica(This,replica,callback,context) ) 

#define IFabricPrimaryReplicator_EndBuildReplica(This,context)	\
    ( (This)->lpVtbl -> EndBuildReplica(This,context) ) 

#define IFabricPrimaryReplicator_RemoveReplica(This,replicaId)	\
    ( (This)->lpVtbl -> RemoveReplica(This,replicaId) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricPrimaryReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricReplicatorCatchupSpecificQuorum_INTERFACE_DEFINED__
#define __IFabricReplicatorCatchupSpecificQuorum_INTERFACE_DEFINED__

/* interface IFabricReplicatorCatchupSpecificQuorum */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricReplicatorCatchupSpecificQuorum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa3116fe-277d-482d-bd16-5366fa405757")
    IFabricReplicatorCatchupSpecificQuorum : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricReplicatorCatchupSpecificQuorumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricReplicatorCatchupSpecificQuorum * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricReplicatorCatchupSpecificQuorum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricReplicatorCatchupSpecificQuorum * This);
        
        END_INTERFACE
    } IFabricReplicatorCatchupSpecificQuorumVtbl;

    interface IFabricReplicatorCatchupSpecificQuorum
    {
        CONST_VTBL struct IFabricReplicatorCatchupSpecificQuorumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricReplicatorCatchupSpecificQuorum_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricReplicatorCatchupSpecificQuorum_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricReplicatorCatchupSpecificQuorum_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricReplicatorCatchupSpecificQuorum_INTERFACE_DEFINED__ */


#ifndef __IFabricOperation_INTERFACE_DEFINED__
#define __IFabricOperation_INTERFACE_DEFINED__

/* interface IFabricOperation */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricOperation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f4ad6bfa-e23c-4a48-9617-c099cd59a23a")
    IFabricOperation : public IUnknown
    {
    public:
        virtual const FABRIC_OPERATION_METADATA *STDMETHODCALLTYPE get_Metadata( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Acknowledge( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricOperationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricOperation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricOperation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricOperation * This);
        
        const FABRIC_OPERATION_METADATA *( STDMETHODCALLTYPE *get_Metadata )( 
            IFabricOperation * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IFabricOperation * This,
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers);
        
        HRESULT ( STDMETHODCALLTYPE *Acknowledge )( 
            IFabricOperation * This);
        
        END_INTERFACE
    } IFabricOperationVtbl;

    interface IFabricOperation
    {
        CONST_VTBL struct IFabricOperationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricOperation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricOperation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricOperation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricOperation_get_Metadata(This)	\
    ( (This)->lpVtbl -> get_Metadata(This) ) 

#define IFabricOperation_GetData(This,count,buffers)	\
    ( (This)->lpVtbl -> GetData(This,count,buffers) ) 

#define IFabricOperation_Acknowledge(This)	\
    ( (This)->lpVtbl -> Acknowledge(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricOperation_INTERFACE_DEFINED__ */


#ifndef __IFabricOperationData_INTERFACE_DEFINED__
#define __IFabricOperationData_INTERFACE_DEFINED__

/* interface IFabricOperationData */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricOperationData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bab8ad87-37b7-482a-985d-baf38a785dcd")
    IFabricOperationData : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricOperationDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricOperationData * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricOperationData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricOperationData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IFabricOperationData * This,
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers);
        
        END_INTERFACE
    } IFabricOperationDataVtbl;

    interface IFabricOperationData
    {
        CONST_VTBL struct IFabricOperationDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricOperationData_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricOperationData_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricOperationData_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricOperationData_GetData(This,count,buffers)	\
    ( (This)->lpVtbl -> GetData(This,count,buffers) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricOperationData_INTERFACE_DEFINED__ */


#ifndef __IFabricOperationStream_INTERFACE_DEFINED__
#define __IFabricOperationStream_INTERFACE_DEFINED__

/* interface IFabricOperationStream */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricOperationStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A98FB97A-D6B0-408A-A878-A9EDB09C2587")
    IFabricOperationStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginGetOperation( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetOperation( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOperation **operation) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricOperationStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricOperationStream * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricOperationStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricOperationStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetOperation )( 
            IFabricOperationStream * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetOperation )( 
            IFabricOperationStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOperation **operation);
        
        END_INTERFACE
    } IFabricOperationStreamVtbl;

    interface IFabricOperationStream
    {
        CONST_VTBL struct IFabricOperationStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricOperationStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricOperationStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricOperationStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricOperationStream_BeginGetOperation(This,callback,context)	\
    ( (This)->lpVtbl -> BeginGetOperation(This,callback,context) ) 

#define IFabricOperationStream_EndGetOperation(This,context,operation)	\
    ( (This)->lpVtbl -> EndGetOperation(This,context,operation) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricOperationStream_INTERFACE_DEFINED__ */


#ifndef __IFabricOperationStream2_INTERFACE_DEFINED__
#define __IFabricOperationStream2_INTERFACE_DEFINED__

/* interface IFabricOperationStream2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricOperationStream2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0930199B-590A-4065-BEC9-5F93B6AAE086")
    IFabricOperationStream2 : public IFabricOperationStream
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportFault( 
            /* [in] */ FABRIC_FAULT_TYPE faultType) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricOperationStream2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricOperationStream2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricOperationStream2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricOperationStream2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetOperation )( 
            IFabricOperationStream2 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetOperation )( 
            IFabricOperationStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOperation **operation);
        
        HRESULT ( STDMETHODCALLTYPE *ReportFault )( 
            IFabricOperationStream2 * This,
            /* [in] */ FABRIC_FAULT_TYPE faultType);
        
        END_INTERFACE
    } IFabricOperationStream2Vtbl;

    interface IFabricOperationStream2
    {
        CONST_VTBL struct IFabricOperationStream2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricOperationStream2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricOperationStream2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricOperationStream2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricOperationStream2_BeginGetOperation(This,callback,context)	\
    ( (This)->lpVtbl -> BeginGetOperation(This,callback,context) ) 

#define IFabricOperationStream2_EndGetOperation(This,context,operation)	\
    ( (This)->lpVtbl -> EndGetOperation(This,context,operation) ) 


#define IFabricOperationStream2_ReportFault(This,faultType)	\
    ( (This)->lpVtbl -> ReportFault(This,faultType) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricOperationStream2_INTERFACE_DEFINED__ */


#ifndef __IFabricOperationDataStream_INTERFACE_DEFINED__
#define __IFabricOperationDataStream_INTERFACE_DEFINED__

/* interface IFabricOperationDataStream */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricOperationDataStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c4e9084c-be92-49c9-8c18-d44d088c2e32")
    IFabricOperationDataStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginGetNext( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetNext( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOperationData **operationData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricOperationDataStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricOperationDataStream * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricOperationDataStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricOperationDataStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetNext )( 
            IFabricOperationDataStream * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetNext )( 
            IFabricOperationDataStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOperationData **operationData);
        
        END_INTERFACE
    } IFabricOperationDataStreamVtbl;

    interface IFabricOperationDataStream
    {
        CONST_VTBL struct IFabricOperationDataStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricOperationDataStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricOperationDataStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricOperationDataStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricOperationDataStream_BeginGetNext(This,callback,context)	\
    ( (This)->lpVtbl -> BeginGetNext(This,callback,context) ) 

#define IFabricOperationDataStream_EndGetNext(This,context,operationData)	\
    ( (This)->lpVtbl -> EndGetNext(This,context,operationData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricOperationDataStream_INTERFACE_DEFINED__ */


#ifndef __IFabricAtomicGroupStateProvider_INTERFACE_DEFINED__
#define __IFabricAtomicGroupStateProvider_INTERFACE_DEFINED__

/* interface IFabricAtomicGroupStateProvider */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricAtomicGroupStateProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2b670953-6148-4f7d-a920-b390de43d913")
    IFabricAtomicGroupStateProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginAtomicGroupCommit( 
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAtomicGroupCommit( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAtomicGroupRollback( 
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ FABRIC_SEQUENCE_NUMBER rollbackequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAtomicGroupRollback( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUndoProgress( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUndoProgress( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricAtomicGroupStateProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricAtomicGroupStateProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricAtomicGroupStateProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAtomicGroupCommit )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndAtomicGroupCommit )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAtomicGroupRollback )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ FABRIC_SEQUENCE_NUMBER rollbackequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndAtomicGroupRollback )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUndoProgress )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUndoProgress )( 
            IFabricAtomicGroupStateProvider * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricAtomicGroupStateProviderVtbl;

    interface IFabricAtomicGroupStateProvider
    {
        CONST_VTBL struct IFabricAtomicGroupStateProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricAtomicGroupStateProvider_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricAtomicGroupStateProvider_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricAtomicGroupStateProvider_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricAtomicGroupStateProvider_BeginAtomicGroupCommit(This,atomicGroupId,commitSequenceNumber,callback,context)	\
    ( (This)->lpVtbl -> BeginAtomicGroupCommit(This,atomicGroupId,commitSequenceNumber,callback,context) ) 

#define IFabricAtomicGroupStateProvider_EndAtomicGroupCommit(This,context)	\
    ( (This)->lpVtbl -> EndAtomicGroupCommit(This,context) ) 

#define IFabricAtomicGroupStateProvider_BeginAtomicGroupRollback(This,atomicGroupId,rollbackequenceNumber,callback,context)	\
    ( (This)->lpVtbl -> BeginAtomicGroupRollback(This,atomicGroupId,rollbackequenceNumber,callback,context) ) 

#define IFabricAtomicGroupStateProvider_EndAtomicGroupRollback(This,context)	\
    ( (This)->lpVtbl -> EndAtomicGroupRollback(This,context) ) 

#define IFabricAtomicGroupStateProvider_BeginUndoProgress(This,fromCommitSequenceNumber,callback,context)	\
    ( (This)->lpVtbl -> BeginUndoProgress(This,fromCommitSequenceNumber,callback,context) ) 

#define IFabricAtomicGroupStateProvider_EndUndoProgress(This,context)	\
    ( (This)->lpVtbl -> EndUndoProgress(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricAtomicGroupStateProvider_INTERFACE_DEFINED__ */


#ifndef __IFabricAtomicGroupStateReplicator_INTERFACE_DEFINED__
#define __IFabricAtomicGroupStateReplicator_INTERFACE_DEFINED__

/* interface IFabricAtomicGroupStateReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricAtomicGroupStateReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80d2155c-4fc2-4fde-9696-c2f39b471c3d")
    IFabricAtomicGroupStateReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateAtomicGroup( 
            /* [retval][out] */ FABRIC_ATOMIC_GROUP_ID *AtomicGroupId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReplicateAtomicGroupOperation( 
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ IFabricOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *operationSequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReplicateAtomicGroupOperation( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *operationSequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReplicateAtomicGroupCommit( 
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReplicateAtomicGroupCommit( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReplicateAtomicGroupRollback( 
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *rollbackSequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReplicateAtomicGroupRollback( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *rollbackSequenceNumber) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricAtomicGroupStateReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricAtomicGroupStateReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricAtomicGroupStateReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAtomicGroup )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [retval][out] */ FABRIC_ATOMIC_GROUP_ID *AtomicGroupId);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicateAtomicGroupOperation )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ IFabricOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *operationSequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReplicateAtomicGroupOperation )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *operationSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicateAtomicGroupCommit )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReplicateAtomicGroupCommit )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicateAtomicGroupRollback )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *rollbackSequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReplicateAtomicGroupRollback )( 
            IFabricAtomicGroupStateReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *rollbackSequenceNumber);
        
        END_INTERFACE
    } IFabricAtomicGroupStateReplicatorVtbl;

    interface IFabricAtomicGroupStateReplicator
    {
        CONST_VTBL struct IFabricAtomicGroupStateReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricAtomicGroupStateReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricAtomicGroupStateReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricAtomicGroupStateReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricAtomicGroupStateReplicator_CreateAtomicGroup(This,AtomicGroupId)	\
    ( (This)->lpVtbl -> CreateAtomicGroup(This,AtomicGroupId) ) 

#define IFabricAtomicGroupStateReplicator_BeginReplicateAtomicGroupOperation(This,atomicGroupId,operationData,callback,operationSequenceNumber,context)	\
    ( (This)->lpVtbl -> BeginReplicateAtomicGroupOperation(This,atomicGroupId,operationData,callback,operationSequenceNumber,context) ) 

#define IFabricAtomicGroupStateReplicator_EndReplicateAtomicGroupOperation(This,context,operationSequenceNumber)	\
    ( (This)->lpVtbl -> EndReplicateAtomicGroupOperation(This,context,operationSequenceNumber) ) 

#define IFabricAtomicGroupStateReplicator_BeginReplicateAtomicGroupCommit(This,atomicGroupId,callback,commitSequenceNumber,context)	\
    ( (This)->lpVtbl -> BeginReplicateAtomicGroupCommit(This,atomicGroupId,callback,commitSequenceNumber,context) ) 

#define IFabricAtomicGroupStateReplicator_EndReplicateAtomicGroupCommit(This,context,commitSequenceNumber)	\
    ( (This)->lpVtbl -> EndReplicateAtomicGroupCommit(This,context,commitSequenceNumber) ) 

#define IFabricAtomicGroupStateReplicator_BeginReplicateAtomicGroupRollback(This,atomicGroupId,callback,rollbackSequenceNumber,context)	\
    ( (This)->lpVtbl -> BeginReplicateAtomicGroupRollback(This,atomicGroupId,callback,rollbackSequenceNumber,context) ) 

#define IFabricAtomicGroupStateReplicator_EndReplicateAtomicGroupRollback(This,context,rollbackSequenceNumber)	\
    ( (This)->lpVtbl -> EndReplicateAtomicGroupRollback(This,context,rollbackSequenceNumber) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricAtomicGroupStateReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceGroupFactory_INTERFACE_DEFINED__
#define __IFabricServiceGroupFactory_INTERFACE_DEFINED__

/* interface IFabricServiceGroupFactory */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceGroupFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3860d61d-1e51-4a65-b109-d93c11311657")
    IFabricServiceGroupFactory : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceGroupFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceGroupFactory * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceGroupFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceGroupFactory * This);
        
        END_INTERFACE
    } IFabricServiceGroupFactoryVtbl;

    interface IFabricServiceGroupFactory
    {
        CONST_VTBL struct IFabricServiceGroupFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceGroupFactory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceGroupFactory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceGroupFactory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceGroupFactory_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceGroupFactoryBuilder_INTERFACE_DEFINED__
#define __IFabricServiceGroupFactoryBuilder_INTERFACE_DEFINED__

/* interface IFabricServiceGroupFactoryBuilder */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceGroupFactoryBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a9fe8b06-19b1-49e6-8911-41d9d9219e1c")
    IFabricServiceGroupFactoryBuilder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddStatelessServiceFactory( 
            /* [in] */ LPCWSTR memberServiceType,
            /* [in] */ IFabricStatelessServiceFactory *factory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddStatefulServiceFactory( 
            /* [in] */ LPCWSTR memberServiceType,
            /* [in] */ IFabricStatefulServiceFactory *factory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveServiceFactory( 
            /* [in] */ LPCWSTR memberServiceType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ToServiceGroupFactory( 
            /* [retval][out] */ IFabricServiceGroupFactory **factory) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceGroupFactoryBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceGroupFactoryBuilder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceGroupFactoryBuilder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceGroupFactoryBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddStatelessServiceFactory )( 
            IFabricServiceGroupFactoryBuilder * This,
            /* [in] */ LPCWSTR memberServiceType,
            /* [in] */ IFabricStatelessServiceFactory *factory);
        
        HRESULT ( STDMETHODCALLTYPE *AddStatefulServiceFactory )( 
            IFabricServiceGroupFactoryBuilder * This,
            /* [in] */ LPCWSTR memberServiceType,
            /* [in] */ IFabricStatefulServiceFactory *factory);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveServiceFactory )( 
            IFabricServiceGroupFactoryBuilder * This,
            /* [in] */ LPCWSTR memberServiceType);
        
        HRESULT ( STDMETHODCALLTYPE *ToServiceGroupFactory )( 
            IFabricServiceGroupFactoryBuilder * This,
            /* [retval][out] */ IFabricServiceGroupFactory **factory);
        
        END_INTERFACE
    } IFabricServiceGroupFactoryBuilderVtbl;

    interface IFabricServiceGroupFactoryBuilder
    {
        CONST_VTBL struct IFabricServiceGroupFactoryBuilderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceGroupFactoryBuilder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceGroupFactoryBuilder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceGroupFactoryBuilder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceGroupFactoryBuilder_AddStatelessServiceFactory(This,memberServiceType,factory)	\
    ( (This)->lpVtbl -> AddStatelessServiceFactory(This,memberServiceType,factory) ) 

#define IFabricServiceGroupFactoryBuilder_AddStatefulServiceFactory(This,memberServiceType,factory)	\
    ( (This)->lpVtbl -> AddStatefulServiceFactory(This,memberServiceType,factory) ) 

#define IFabricServiceGroupFactoryBuilder_RemoveServiceFactory(This,memberServiceType)	\
    ( (This)->lpVtbl -> RemoveServiceFactory(This,memberServiceType) ) 

#define IFabricServiceGroupFactoryBuilder_ToServiceGroupFactory(This,factory)	\
    ( (This)->lpVtbl -> ToServiceGroupFactory(This,factory) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceGroupFactoryBuilder_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceGroupPartition_INTERFACE_DEFINED__
#define __IFabricServiceGroupPartition_INTERFACE_DEFINED__

/* interface IFabricServiceGroupPartition */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceGroupPartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2b24299a-7489-467f-8e7f-4507bff73b86")
    IFabricServiceGroupPartition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ResolveMember( 
            /* [in] */ FABRIC_URI name,
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **member) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceGroupPartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceGroupPartition * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceGroupPartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceGroupPartition * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResolveMember )( 
            IFabricServiceGroupPartition * This,
            /* [in] */ FABRIC_URI name,
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **member);
        
        END_INTERFACE
    } IFabricServiceGroupPartitionVtbl;

    interface IFabricServiceGroupPartition
    {
        CONST_VTBL struct IFabricServiceGroupPartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceGroupPartition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceGroupPartition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceGroupPartition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceGroupPartition_ResolveMember(This,name,riid,member)	\
    ( (This)->lpVtbl -> ResolveMember(This,name,riid,member) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceGroupPartition_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext_INTERFACE_DEFINED__
#define __IFabricCodePackageActivationContext_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivationContext */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivationContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("68a971e2-f15f-4d95-a79c-8a257909659e")
    IFabricCodePackageActivationContext : public IUnknown
    {
    public:
        virtual LPCWSTR STDMETHODCALLTYPE get_ContextId( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_CodePackageName( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_CodePackageVersion( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_WorkDirectory( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_LogDirectory( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_TempDirectory( void) = 0;
        
        virtual const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *STDMETHODCALLTYPE get_ServiceTypes( void) = 0;
        
        virtual const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *STDMETHODCALLTYPE get_ServiceGroupTypes( void) = 0;
        
        virtual const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *STDMETHODCALLTYPE get_ApplicationPrincipals( void) = 0;
        
        virtual const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *STDMETHODCALLTYPE get_ServiceEndpointResources( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceEndpointResource( 
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackageNames( 
            /* [retval][out] */ IFabricStringListResult **names) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigurationPackageNames( 
            /* [retval][out] */ IFabricStringListResult **names) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackageNames( 
            /* [retval][out] */ IFabricStringListResult **names) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodePackage( 
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigurationPackage( 
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDataPackage( 
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterCodePackageChangeHandler( 
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterCodePackageChangeHandler( 
            /* [in] */ LONGLONG callbackHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterConfigurationPackageChangeHandler( 
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterConfigurationPackageChangeHandler( 
            /* [in] */ LONGLONG callbackHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterDataPackageChangeHandler( 
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterDataPackageChangeHandler( 
            /* [in] */ LONGLONG callbackHandle) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivationContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivationContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivationContext * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ContextId )( 
            IFabricCodePackageActivationContext * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageName )( 
            IFabricCodePackageActivationContext * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageVersion )( 
            IFabricCodePackageActivationContext * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricCodePackageActivationContext * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_LogDirectory )( 
            IFabricCodePackageActivationContext * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_TempDirectory )( 
            IFabricCodePackageActivationContext * This);
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceTypes )( 
            IFabricCodePackageActivationContext * This);
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceGroupTypes )( 
            IFabricCodePackageActivationContext * This);
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *( STDMETHODCALLTYPE *get_ApplicationPrincipals )( 
            IFabricCodePackageActivationContext * This);
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceEndpointResources )( 
            IFabricCodePackageActivationContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceEndpointResource )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageNames )( 
            IFabricCodePackageActivationContext * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackageNames )( 
            IFabricCodePackageActivationContext * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageNames )( 
            IFabricCodePackageActivationContext * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackage )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackage )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackage )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext * This,
            /* [in] */ LONGLONG callbackHandle);
        
        END_INTERFACE
    } IFabricCodePackageActivationContextVtbl;

    interface IFabricCodePackageActivationContext
    {
        CONST_VTBL struct IFabricCodePackageActivationContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivationContext_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivationContext_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivationContext_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivationContext_get_ContextId(This)	\
    ( (This)->lpVtbl -> get_ContextId(This) ) 

#define IFabricCodePackageActivationContext_get_CodePackageName(This)	\
    ( (This)->lpVtbl -> get_CodePackageName(This) ) 

#define IFabricCodePackageActivationContext_get_CodePackageVersion(This)	\
    ( (This)->lpVtbl -> get_CodePackageVersion(This) ) 

#define IFabricCodePackageActivationContext_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#define IFabricCodePackageActivationContext_get_LogDirectory(This)	\
    ( (This)->lpVtbl -> get_LogDirectory(This) ) 

#define IFabricCodePackageActivationContext_get_TempDirectory(This)	\
    ( (This)->lpVtbl -> get_TempDirectory(This) ) 

#define IFabricCodePackageActivationContext_get_ServiceTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceTypes(This) ) 

#define IFabricCodePackageActivationContext_get_ServiceGroupTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceGroupTypes(This) ) 

#define IFabricCodePackageActivationContext_get_ApplicationPrincipals(This)	\
    ( (This)->lpVtbl -> get_ApplicationPrincipals(This) ) 

#define IFabricCodePackageActivationContext_get_ServiceEndpointResources(This)	\
    ( (This)->lpVtbl -> get_ServiceEndpointResources(This) ) 

#define IFabricCodePackageActivationContext_GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue)	\
    ( (This)->lpVtbl -> GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue) ) 

#define IFabricCodePackageActivationContext_GetCodePackageNames(This,names)	\
    ( (This)->lpVtbl -> GetCodePackageNames(This,names) ) 

#define IFabricCodePackageActivationContext_GetConfigurationPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetConfigurationPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext_GetDataPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetDataPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext_GetCodePackage(This,codePackageName,codePackage)	\
    ( (This)->lpVtbl -> GetCodePackage(This,codePackageName,codePackage) ) 

#define IFabricCodePackageActivationContext_GetConfigurationPackage(This,configPackageName,configPackage)	\
    ( (This)->lpVtbl -> GetConfigurationPackage(This,configPackageName,configPackage) ) 

#define IFabricCodePackageActivationContext_GetDataPackage(This,dataPackageName,dataPackage)	\
    ( (This)->lpVtbl -> GetDataPackage(This,dataPackageName,dataPackage) ) 

#define IFabricCodePackageActivationContext_RegisterCodePackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext_UnregisterCodePackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext_RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext_UnregisterConfigurationPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterConfigurationPackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext_RegisterDataPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterDataPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext_UnregisterDataPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterDataPackageChangeHandler(This,callbackHandle) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivationContext_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext2_INTERFACE_DEFINED__
#define __IFabricCodePackageActivationContext2_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivationContext2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivationContext2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6c83d5c1-1954-4b80-9175-0d0e7c8715c9")
    IFabricCodePackageActivationContext2 : public IFabricCodePackageActivationContext
    {
    public:
        virtual FABRIC_URI STDMETHODCALLTYPE get_ApplicationName( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_ApplicationTypeName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestName( 
            /* [retval][out] */ IFabricStringResult **serviceManifestName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServiceManifestVersion( 
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivationContext2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivationContext2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ContextId )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageName )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageVersion )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_LogDirectory )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_TempDirectory )( 
            IFabricCodePackageActivationContext2 * This);
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceTypes )( 
            IFabricCodePackageActivationContext2 * This);
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceGroupTypes )( 
            IFabricCodePackageActivationContext2 * This);
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *( STDMETHODCALLTYPE *get_ApplicationPrincipals )( 
            IFabricCodePackageActivationContext2 * This);
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceEndpointResources )( 
            IFabricCodePackageActivationContext2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceEndpointResource )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageNames )( 
            IFabricCodePackageActivationContext2 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackageNames )( 
            IFabricCodePackageActivationContext2 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageNames )( 
            IFabricCodePackageActivationContext2 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackage )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackage )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackage )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext2 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        FABRIC_URI ( STDMETHODCALLTYPE *get_ApplicationName )( 
            IFabricCodePackageActivationContext2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ApplicationTypeName )( 
            IFabricCodePackageActivationContext2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestName )( 
            IFabricCodePackageActivationContext2 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestVersion )( 
            IFabricCodePackageActivationContext2 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion);
        
        END_INTERFACE
    } IFabricCodePackageActivationContext2Vtbl;

    interface IFabricCodePackageActivationContext2
    {
        CONST_VTBL struct IFabricCodePackageActivationContext2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivationContext2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivationContext2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivationContext2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivationContext2_get_ContextId(This)	\
    ( (This)->lpVtbl -> get_ContextId(This) ) 

#define IFabricCodePackageActivationContext2_get_CodePackageName(This)	\
    ( (This)->lpVtbl -> get_CodePackageName(This) ) 

#define IFabricCodePackageActivationContext2_get_CodePackageVersion(This)	\
    ( (This)->lpVtbl -> get_CodePackageVersion(This) ) 

#define IFabricCodePackageActivationContext2_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#define IFabricCodePackageActivationContext2_get_LogDirectory(This)	\
    ( (This)->lpVtbl -> get_LogDirectory(This) ) 

#define IFabricCodePackageActivationContext2_get_TempDirectory(This)	\
    ( (This)->lpVtbl -> get_TempDirectory(This) ) 

#define IFabricCodePackageActivationContext2_get_ServiceTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceTypes(This) ) 

#define IFabricCodePackageActivationContext2_get_ServiceGroupTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceGroupTypes(This) ) 

#define IFabricCodePackageActivationContext2_get_ApplicationPrincipals(This)	\
    ( (This)->lpVtbl -> get_ApplicationPrincipals(This) ) 

#define IFabricCodePackageActivationContext2_get_ServiceEndpointResources(This)	\
    ( (This)->lpVtbl -> get_ServiceEndpointResources(This) ) 

#define IFabricCodePackageActivationContext2_GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue)	\
    ( (This)->lpVtbl -> GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue) ) 

#define IFabricCodePackageActivationContext2_GetCodePackageNames(This,names)	\
    ( (This)->lpVtbl -> GetCodePackageNames(This,names) ) 

#define IFabricCodePackageActivationContext2_GetConfigurationPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetConfigurationPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext2_GetDataPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetDataPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext2_GetCodePackage(This,codePackageName,codePackage)	\
    ( (This)->lpVtbl -> GetCodePackage(This,codePackageName,codePackage) ) 

#define IFabricCodePackageActivationContext2_GetConfigurationPackage(This,configPackageName,configPackage)	\
    ( (This)->lpVtbl -> GetConfigurationPackage(This,configPackageName,configPackage) ) 

#define IFabricCodePackageActivationContext2_GetDataPackage(This,dataPackageName,dataPackage)	\
    ( (This)->lpVtbl -> GetDataPackage(This,dataPackageName,dataPackage) ) 

#define IFabricCodePackageActivationContext2_RegisterCodePackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext2_UnregisterCodePackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext2_RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext2_UnregisterConfigurationPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterConfigurationPackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext2_RegisterDataPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterDataPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext2_UnregisterDataPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterDataPackageChangeHandler(This,callbackHandle) ) 


#define IFabricCodePackageActivationContext2_get_ApplicationName(This)	\
    ( (This)->lpVtbl -> get_ApplicationName(This) ) 

#define IFabricCodePackageActivationContext2_get_ApplicationTypeName(This)	\
    ( (This)->lpVtbl -> get_ApplicationTypeName(This) ) 

#define IFabricCodePackageActivationContext2_GetServiceManifestName(This,serviceManifestName)	\
    ( (This)->lpVtbl -> GetServiceManifestName(This,serviceManifestName) ) 

#define IFabricCodePackageActivationContext2_GetServiceManifestVersion(This,serviceManifestVersion)	\
    ( (This)->lpVtbl -> GetServiceManifestVersion(This,serviceManifestVersion) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivationContext2_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext3_INTERFACE_DEFINED__
#define __IFabricCodePackageActivationContext3_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivationContext3 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivationContext3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6efee900-f491-4b03-bc5b-3a70de103593")
    IFabricCodePackageActivationContext3 : public IFabricCodePackageActivationContext2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportApplicationHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportDeployedApplicationHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportDeployedServicePackageHealth( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivationContext3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivationContext3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ContextId )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageName )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageVersion )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_LogDirectory )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_TempDirectory )( 
            IFabricCodePackageActivationContext3 * This);
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceTypes )( 
            IFabricCodePackageActivationContext3 * This);
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceGroupTypes )( 
            IFabricCodePackageActivationContext3 * This);
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *( STDMETHODCALLTYPE *get_ApplicationPrincipals )( 
            IFabricCodePackageActivationContext3 * This);
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceEndpointResources )( 
            IFabricCodePackageActivationContext3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceEndpointResource )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageNames )( 
            IFabricCodePackageActivationContext3 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackageNames )( 
            IFabricCodePackageActivationContext3 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageNames )( 
            IFabricCodePackageActivationContext3 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackage )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackage )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackage )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        FABRIC_URI ( STDMETHODCALLTYPE *get_ApplicationName )( 
            IFabricCodePackageActivationContext3 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ApplicationTypeName )( 
            IFabricCodePackageActivationContext3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestName )( 
            IFabricCodePackageActivationContext3 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestVersion )( 
            IFabricCodePackageActivationContext3 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth )( 
            IFabricCodePackageActivationContext3 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        END_INTERFACE
    } IFabricCodePackageActivationContext3Vtbl;

    interface IFabricCodePackageActivationContext3
    {
        CONST_VTBL struct IFabricCodePackageActivationContext3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivationContext3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivationContext3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivationContext3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivationContext3_get_ContextId(This)	\
    ( (This)->lpVtbl -> get_ContextId(This) ) 

#define IFabricCodePackageActivationContext3_get_CodePackageName(This)	\
    ( (This)->lpVtbl -> get_CodePackageName(This) ) 

#define IFabricCodePackageActivationContext3_get_CodePackageVersion(This)	\
    ( (This)->lpVtbl -> get_CodePackageVersion(This) ) 

#define IFabricCodePackageActivationContext3_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#define IFabricCodePackageActivationContext3_get_LogDirectory(This)	\
    ( (This)->lpVtbl -> get_LogDirectory(This) ) 

#define IFabricCodePackageActivationContext3_get_TempDirectory(This)	\
    ( (This)->lpVtbl -> get_TempDirectory(This) ) 

#define IFabricCodePackageActivationContext3_get_ServiceTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceTypes(This) ) 

#define IFabricCodePackageActivationContext3_get_ServiceGroupTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceGroupTypes(This) ) 

#define IFabricCodePackageActivationContext3_get_ApplicationPrincipals(This)	\
    ( (This)->lpVtbl -> get_ApplicationPrincipals(This) ) 

#define IFabricCodePackageActivationContext3_get_ServiceEndpointResources(This)	\
    ( (This)->lpVtbl -> get_ServiceEndpointResources(This) ) 

#define IFabricCodePackageActivationContext3_GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue)	\
    ( (This)->lpVtbl -> GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue) ) 

#define IFabricCodePackageActivationContext3_GetCodePackageNames(This,names)	\
    ( (This)->lpVtbl -> GetCodePackageNames(This,names) ) 

#define IFabricCodePackageActivationContext3_GetConfigurationPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetConfigurationPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext3_GetDataPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetDataPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext3_GetCodePackage(This,codePackageName,codePackage)	\
    ( (This)->lpVtbl -> GetCodePackage(This,codePackageName,codePackage) ) 

#define IFabricCodePackageActivationContext3_GetConfigurationPackage(This,configPackageName,configPackage)	\
    ( (This)->lpVtbl -> GetConfigurationPackage(This,configPackageName,configPackage) ) 

#define IFabricCodePackageActivationContext3_GetDataPackage(This,dataPackageName,dataPackage)	\
    ( (This)->lpVtbl -> GetDataPackage(This,dataPackageName,dataPackage) ) 

#define IFabricCodePackageActivationContext3_RegisterCodePackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext3_UnregisterCodePackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext3_RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext3_UnregisterConfigurationPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterConfigurationPackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext3_RegisterDataPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterDataPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext3_UnregisterDataPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterDataPackageChangeHandler(This,callbackHandle) ) 


#define IFabricCodePackageActivationContext3_get_ApplicationName(This)	\
    ( (This)->lpVtbl -> get_ApplicationName(This) ) 

#define IFabricCodePackageActivationContext3_get_ApplicationTypeName(This)	\
    ( (This)->lpVtbl -> get_ApplicationTypeName(This) ) 

#define IFabricCodePackageActivationContext3_GetServiceManifestName(This,serviceManifestName)	\
    ( (This)->lpVtbl -> GetServiceManifestName(This,serviceManifestName) ) 

#define IFabricCodePackageActivationContext3_GetServiceManifestVersion(This,serviceManifestVersion)	\
    ( (This)->lpVtbl -> GetServiceManifestVersion(This,serviceManifestVersion) ) 


#define IFabricCodePackageActivationContext3_ReportApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext3_ReportDeployedApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext3_ReportDeployedServicePackageHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth(This,healthInfo) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivationContext3_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext4_INTERFACE_DEFINED__
#define __IFabricCodePackageActivationContext4_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivationContext4 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivationContext4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("99efebb6-a7b4-4d45-b45e-f191a66eef03")
    IFabricCodePackageActivationContext4 : public IFabricCodePackageActivationContext3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportApplicationHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportDeployedApplicationHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportDeployedServicePackageHealth2( 
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivationContext4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivationContext4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ContextId )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageName )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageVersion )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_LogDirectory )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_TempDirectory )( 
            IFabricCodePackageActivationContext4 * This);
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceTypes )( 
            IFabricCodePackageActivationContext4 * This);
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceGroupTypes )( 
            IFabricCodePackageActivationContext4 * This);
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *( STDMETHODCALLTYPE *get_ApplicationPrincipals )( 
            IFabricCodePackageActivationContext4 * This);
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceEndpointResources )( 
            IFabricCodePackageActivationContext4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceEndpointResource )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageNames )( 
            IFabricCodePackageActivationContext4 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackageNames )( 
            IFabricCodePackageActivationContext4 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageNames )( 
            IFabricCodePackageActivationContext4 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackage )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackage )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackage )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        FABRIC_URI ( STDMETHODCALLTYPE *get_ApplicationName )( 
            IFabricCodePackageActivationContext4 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ApplicationTypeName )( 
            IFabricCodePackageActivationContext4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestName )( 
            IFabricCodePackageActivationContext4 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestVersion )( 
            IFabricCodePackageActivationContext4 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth2 )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth2 )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth2 )( 
            IFabricCodePackageActivationContext4 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        END_INTERFACE
    } IFabricCodePackageActivationContext4Vtbl;

    interface IFabricCodePackageActivationContext4
    {
        CONST_VTBL struct IFabricCodePackageActivationContext4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivationContext4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivationContext4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivationContext4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivationContext4_get_ContextId(This)	\
    ( (This)->lpVtbl -> get_ContextId(This) ) 

#define IFabricCodePackageActivationContext4_get_CodePackageName(This)	\
    ( (This)->lpVtbl -> get_CodePackageName(This) ) 

#define IFabricCodePackageActivationContext4_get_CodePackageVersion(This)	\
    ( (This)->lpVtbl -> get_CodePackageVersion(This) ) 

#define IFabricCodePackageActivationContext4_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#define IFabricCodePackageActivationContext4_get_LogDirectory(This)	\
    ( (This)->lpVtbl -> get_LogDirectory(This) ) 

#define IFabricCodePackageActivationContext4_get_TempDirectory(This)	\
    ( (This)->lpVtbl -> get_TempDirectory(This) ) 

#define IFabricCodePackageActivationContext4_get_ServiceTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceTypes(This) ) 

#define IFabricCodePackageActivationContext4_get_ServiceGroupTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceGroupTypes(This) ) 

#define IFabricCodePackageActivationContext4_get_ApplicationPrincipals(This)	\
    ( (This)->lpVtbl -> get_ApplicationPrincipals(This) ) 

#define IFabricCodePackageActivationContext4_get_ServiceEndpointResources(This)	\
    ( (This)->lpVtbl -> get_ServiceEndpointResources(This) ) 

#define IFabricCodePackageActivationContext4_GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue)	\
    ( (This)->lpVtbl -> GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue) ) 

#define IFabricCodePackageActivationContext4_GetCodePackageNames(This,names)	\
    ( (This)->lpVtbl -> GetCodePackageNames(This,names) ) 

#define IFabricCodePackageActivationContext4_GetConfigurationPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetConfigurationPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext4_GetDataPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetDataPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext4_GetCodePackage(This,codePackageName,codePackage)	\
    ( (This)->lpVtbl -> GetCodePackage(This,codePackageName,codePackage) ) 

#define IFabricCodePackageActivationContext4_GetConfigurationPackage(This,configPackageName,configPackage)	\
    ( (This)->lpVtbl -> GetConfigurationPackage(This,configPackageName,configPackage) ) 

#define IFabricCodePackageActivationContext4_GetDataPackage(This,dataPackageName,dataPackage)	\
    ( (This)->lpVtbl -> GetDataPackage(This,dataPackageName,dataPackage) ) 

#define IFabricCodePackageActivationContext4_RegisterCodePackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext4_UnregisterCodePackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext4_RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext4_UnregisterConfigurationPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterConfigurationPackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext4_RegisterDataPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterDataPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext4_UnregisterDataPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterDataPackageChangeHandler(This,callbackHandle) ) 


#define IFabricCodePackageActivationContext4_get_ApplicationName(This)	\
    ( (This)->lpVtbl -> get_ApplicationName(This) ) 

#define IFabricCodePackageActivationContext4_get_ApplicationTypeName(This)	\
    ( (This)->lpVtbl -> get_ApplicationTypeName(This) ) 

#define IFabricCodePackageActivationContext4_GetServiceManifestName(This,serviceManifestName)	\
    ( (This)->lpVtbl -> GetServiceManifestName(This,serviceManifestName) ) 

#define IFabricCodePackageActivationContext4_GetServiceManifestVersion(This,serviceManifestVersion)	\
    ( (This)->lpVtbl -> GetServiceManifestVersion(This,serviceManifestVersion) ) 


#define IFabricCodePackageActivationContext4_ReportApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext4_ReportDeployedApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext4_ReportDeployedServicePackageHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth(This,healthInfo) ) 


#define IFabricCodePackageActivationContext4_ReportApplicationHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportApplicationHealth2(This,healthInfo,sendOptions) ) 

#define IFabricCodePackageActivationContext4_ReportDeployedApplicationHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth2(This,healthInfo,sendOptions) ) 

#define IFabricCodePackageActivationContext4_ReportDeployedServicePackageHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth2(This,healthInfo,sendOptions) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivationContext4_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext5_INTERFACE_DEFINED__
#define __IFabricCodePackageActivationContext5_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivationContext5 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivationContext5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fe45387e-8711-4949-ac36-31dc95035513")
    IFabricCodePackageActivationContext5 : public IFabricCodePackageActivationContext4
    {
    public:
        virtual LPCWSTR STDMETHODCALLTYPE get_ServiceListenAddress( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_ServicePublishAddress( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivationContext5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivationContext5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ContextId )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageName )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageVersion )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_LogDirectory )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_TempDirectory )( 
            IFabricCodePackageActivationContext5 * This);
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceTypes )( 
            IFabricCodePackageActivationContext5 * This);
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceGroupTypes )( 
            IFabricCodePackageActivationContext5 * This);
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *( STDMETHODCALLTYPE *get_ApplicationPrincipals )( 
            IFabricCodePackageActivationContext5 * This);
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceEndpointResources )( 
            IFabricCodePackageActivationContext5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceEndpointResource )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageNames )( 
            IFabricCodePackageActivationContext5 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackageNames )( 
            IFabricCodePackageActivationContext5 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageNames )( 
            IFabricCodePackageActivationContext5 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackage )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackage )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackage )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        FABRIC_URI ( STDMETHODCALLTYPE *get_ApplicationName )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ApplicationTypeName )( 
            IFabricCodePackageActivationContext5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestName )( 
            IFabricCodePackageActivationContext5 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestVersion )( 
            IFabricCodePackageActivationContext5 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth2 )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth2 )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth2 )( 
            IFabricCodePackageActivationContext5 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ServiceListenAddress )( 
            IFabricCodePackageActivationContext5 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ServicePublishAddress )( 
            IFabricCodePackageActivationContext5 * This);
        
        END_INTERFACE
    } IFabricCodePackageActivationContext5Vtbl;

    interface IFabricCodePackageActivationContext5
    {
        CONST_VTBL struct IFabricCodePackageActivationContext5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivationContext5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivationContext5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivationContext5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivationContext5_get_ContextId(This)	\
    ( (This)->lpVtbl -> get_ContextId(This) ) 

#define IFabricCodePackageActivationContext5_get_CodePackageName(This)	\
    ( (This)->lpVtbl -> get_CodePackageName(This) ) 

#define IFabricCodePackageActivationContext5_get_CodePackageVersion(This)	\
    ( (This)->lpVtbl -> get_CodePackageVersion(This) ) 

#define IFabricCodePackageActivationContext5_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#define IFabricCodePackageActivationContext5_get_LogDirectory(This)	\
    ( (This)->lpVtbl -> get_LogDirectory(This) ) 

#define IFabricCodePackageActivationContext5_get_TempDirectory(This)	\
    ( (This)->lpVtbl -> get_TempDirectory(This) ) 

#define IFabricCodePackageActivationContext5_get_ServiceTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceTypes(This) ) 

#define IFabricCodePackageActivationContext5_get_ServiceGroupTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceGroupTypes(This) ) 

#define IFabricCodePackageActivationContext5_get_ApplicationPrincipals(This)	\
    ( (This)->lpVtbl -> get_ApplicationPrincipals(This) ) 

#define IFabricCodePackageActivationContext5_get_ServiceEndpointResources(This)	\
    ( (This)->lpVtbl -> get_ServiceEndpointResources(This) ) 

#define IFabricCodePackageActivationContext5_GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue)	\
    ( (This)->lpVtbl -> GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue) ) 

#define IFabricCodePackageActivationContext5_GetCodePackageNames(This,names)	\
    ( (This)->lpVtbl -> GetCodePackageNames(This,names) ) 

#define IFabricCodePackageActivationContext5_GetConfigurationPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetConfigurationPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext5_GetDataPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetDataPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext5_GetCodePackage(This,codePackageName,codePackage)	\
    ( (This)->lpVtbl -> GetCodePackage(This,codePackageName,codePackage) ) 

#define IFabricCodePackageActivationContext5_GetConfigurationPackage(This,configPackageName,configPackage)	\
    ( (This)->lpVtbl -> GetConfigurationPackage(This,configPackageName,configPackage) ) 

#define IFabricCodePackageActivationContext5_GetDataPackage(This,dataPackageName,dataPackage)	\
    ( (This)->lpVtbl -> GetDataPackage(This,dataPackageName,dataPackage) ) 

#define IFabricCodePackageActivationContext5_RegisterCodePackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext5_UnregisterCodePackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext5_RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext5_UnregisterConfigurationPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterConfigurationPackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext5_RegisterDataPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterDataPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext5_UnregisterDataPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterDataPackageChangeHandler(This,callbackHandle) ) 


#define IFabricCodePackageActivationContext5_get_ApplicationName(This)	\
    ( (This)->lpVtbl -> get_ApplicationName(This) ) 

#define IFabricCodePackageActivationContext5_get_ApplicationTypeName(This)	\
    ( (This)->lpVtbl -> get_ApplicationTypeName(This) ) 

#define IFabricCodePackageActivationContext5_GetServiceManifestName(This,serviceManifestName)	\
    ( (This)->lpVtbl -> GetServiceManifestName(This,serviceManifestName) ) 

#define IFabricCodePackageActivationContext5_GetServiceManifestVersion(This,serviceManifestVersion)	\
    ( (This)->lpVtbl -> GetServiceManifestVersion(This,serviceManifestVersion) ) 


#define IFabricCodePackageActivationContext5_ReportApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext5_ReportDeployedApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext5_ReportDeployedServicePackageHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth(This,healthInfo) ) 


#define IFabricCodePackageActivationContext5_ReportApplicationHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportApplicationHealth2(This,healthInfo,sendOptions) ) 

#define IFabricCodePackageActivationContext5_ReportDeployedApplicationHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth2(This,healthInfo,sendOptions) ) 

#define IFabricCodePackageActivationContext5_ReportDeployedServicePackageHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth2(This,healthInfo,sendOptions) ) 


#define IFabricCodePackageActivationContext5_get_ServiceListenAddress(This)	\
    ( (This)->lpVtbl -> get_ServiceListenAddress(This) ) 

#define IFabricCodePackageActivationContext5_get_ServicePublishAddress(This)	\
    ( (This)->lpVtbl -> get_ServicePublishAddress(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivationContext5_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivationContext6_INTERFACE_DEFINED__
#define __IFabricCodePackageActivationContext6_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivationContext6 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivationContext6;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fa5fda9b-472c-45a0-9b60-a374691227a4")
    IFabricCodePackageActivationContext6 : public IFabricCodePackageActivationContext5
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDirectory( 
            /* [in] */ LPCWSTR logicalDirectoryName,
            /* [retval][out] */ IFabricStringResult **directoryPath) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivationContext6Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivationContext6 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ContextId )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageName )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_CodePackageVersion )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_LogDirectory )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_TempDirectory )( 
            IFabricCodePackageActivationContext6 * This);
        
        const FABRIC_SERVICE_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceTypes )( 
            IFabricCodePackageActivationContext6 * This);
        
        const FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceGroupTypes )( 
            IFabricCodePackageActivationContext6 * This);
        
        const FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION *( STDMETHODCALLTYPE *get_ApplicationPrincipals )( 
            IFabricCodePackageActivationContext6 * This);
        
        const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST *( STDMETHODCALLTYPE *get_ServiceEndpointResources )( 
            IFabricCodePackageActivationContext6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceEndpointResource )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LPCWSTR serviceEndpointResourceName,
            /* [retval][out] */ const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackageNames )( 
            IFabricCodePackageActivationContext6 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackageNames )( 
            IFabricCodePackageActivationContext6 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackageNames )( 
            IFabricCodePackageActivationContext6 * This,
            /* [retval][out] */ IFabricStringListResult **names);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodePackage )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LPCWSTR codePackageName,
            /* [retval][out] */ IFabricCodePackage **codePackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationPackage )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LPCWSTR configPackageName,
            /* [retval][out] */ IFabricConfigurationPackage **configPackage);
        
        HRESULT ( STDMETHODCALLTYPE *GetDataPackage )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LPCWSTR dataPackageName,
            /* [retval][out] */ IFabricDataPackage **dataPackage);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ IFabricCodePackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageChangeHandler )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterConfigurationPackageChangeHandler )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ IFabricDataPackageChangeHandler *callback,
            /* [retval][out] */ LONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterDataPackageChangeHandler )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LONGLONG callbackHandle);
        
        FABRIC_URI ( STDMETHODCALLTYPE *get_ApplicationName )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ApplicationTypeName )( 
            IFabricCodePackageActivationContext6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestName )( 
            IFabricCodePackageActivationContext6 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestName);
        
        HRESULT ( STDMETHODCALLTYPE *GetServiceManifestVersion )( 
            IFabricCodePackageActivationContext6 * This,
            /* [retval][out] */ IFabricStringResult **serviceManifestVersion);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *ReportApplicationHealth2 )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedApplicationHealth2 )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        HRESULT ( STDMETHODCALLTYPE *ReportDeployedServicePackageHealth2 )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo,
            /* [in] */ const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ServiceListenAddress )( 
            IFabricCodePackageActivationContext6 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_ServicePublishAddress )( 
            IFabricCodePackageActivationContext6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDirectory )( 
            IFabricCodePackageActivationContext6 * This,
            /* [in] */ LPCWSTR logicalDirectoryName,
            /* [retval][out] */ IFabricStringResult **directoryPath);
        
        END_INTERFACE
    } IFabricCodePackageActivationContext6Vtbl;

    interface IFabricCodePackageActivationContext6
    {
        CONST_VTBL struct IFabricCodePackageActivationContext6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivationContext6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivationContext6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivationContext6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivationContext6_get_ContextId(This)	\
    ( (This)->lpVtbl -> get_ContextId(This) ) 

#define IFabricCodePackageActivationContext6_get_CodePackageName(This)	\
    ( (This)->lpVtbl -> get_CodePackageName(This) ) 

#define IFabricCodePackageActivationContext6_get_CodePackageVersion(This)	\
    ( (This)->lpVtbl -> get_CodePackageVersion(This) ) 

#define IFabricCodePackageActivationContext6_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#define IFabricCodePackageActivationContext6_get_LogDirectory(This)	\
    ( (This)->lpVtbl -> get_LogDirectory(This) ) 

#define IFabricCodePackageActivationContext6_get_TempDirectory(This)	\
    ( (This)->lpVtbl -> get_TempDirectory(This) ) 

#define IFabricCodePackageActivationContext6_get_ServiceTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceTypes(This) ) 

#define IFabricCodePackageActivationContext6_get_ServiceGroupTypes(This)	\
    ( (This)->lpVtbl -> get_ServiceGroupTypes(This) ) 

#define IFabricCodePackageActivationContext6_get_ApplicationPrincipals(This)	\
    ( (This)->lpVtbl -> get_ApplicationPrincipals(This) ) 

#define IFabricCodePackageActivationContext6_get_ServiceEndpointResources(This)	\
    ( (This)->lpVtbl -> get_ServiceEndpointResources(This) ) 

#define IFabricCodePackageActivationContext6_GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue)	\
    ( (This)->lpVtbl -> GetServiceEndpointResource(This,serviceEndpointResourceName,bufferedValue) ) 

#define IFabricCodePackageActivationContext6_GetCodePackageNames(This,names)	\
    ( (This)->lpVtbl -> GetCodePackageNames(This,names) ) 

#define IFabricCodePackageActivationContext6_GetConfigurationPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetConfigurationPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext6_GetDataPackageNames(This,names)	\
    ( (This)->lpVtbl -> GetDataPackageNames(This,names) ) 

#define IFabricCodePackageActivationContext6_GetCodePackage(This,codePackageName,codePackage)	\
    ( (This)->lpVtbl -> GetCodePackage(This,codePackageName,codePackage) ) 

#define IFabricCodePackageActivationContext6_GetConfigurationPackage(This,configPackageName,configPackage)	\
    ( (This)->lpVtbl -> GetConfigurationPackage(This,configPackageName,configPackage) ) 

#define IFabricCodePackageActivationContext6_GetDataPackage(This,dataPackageName,dataPackage)	\
    ( (This)->lpVtbl -> GetDataPackage(This,dataPackageName,dataPackage) ) 

#define IFabricCodePackageActivationContext6_RegisterCodePackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext6_UnregisterCodePackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext6_RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterConfigurationPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext6_UnregisterConfigurationPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterConfigurationPackageChangeHandler(This,callbackHandle) ) 

#define IFabricCodePackageActivationContext6_RegisterDataPackageChangeHandler(This,callback,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterDataPackageChangeHandler(This,callback,callbackHandle) ) 

#define IFabricCodePackageActivationContext6_UnregisterDataPackageChangeHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterDataPackageChangeHandler(This,callbackHandle) ) 


#define IFabricCodePackageActivationContext6_get_ApplicationName(This)	\
    ( (This)->lpVtbl -> get_ApplicationName(This) ) 

#define IFabricCodePackageActivationContext6_get_ApplicationTypeName(This)	\
    ( (This)->lpVtbl -> get_ApplicationTypeName(This) ) 

#define IFabricCodePackageActivationContext6_GetServiceManifestName(This,serviceManifestName)	\
    ( (This)->lpVtbl -> GetServiceManifestName(This,serviceManifestName) ) 

#define IFabricCodePackageActivationContext6_GetServiceManifestVersion(This,serviceManifestVersion)	\
    ( (This)->lpVtbl -> GetServiceManifestVersion(This,serviceManifestVersion) ) 


#define IFabricCodePackageActivationContext6_ReportApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext6_ReportDeployedApplicationHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth(This,healthInfo) ) 

#define IFabricCodePackageActivationContext6_ReportDeployedServicePackageHealth(This,healthInfo)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth(This,healthInfo) ) 


#define IFabricCodePackageActivationContext6_ReportApplicationHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportApplicationHealth2(This,healthInfo,sendOptions) ) 

#define IFabricCodePackageActivationContext6_ReportDeployedApplicationHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportDeployedApplicationHealth2(This,healthInfo,sendOptions) ) 

#define IFabricCodePackageActivationContext6_ReportDeployedServicePackageHealth2(This,healthInfo,sendOptions)	\
    ( (This)->lpVtbl -> ReportDeployedServicePackageHealth2(This,healthInfo,sendOptions) ) 


#define IFabricCodePackageActivationContext6_get_ServiceListenAddress(This)	\
    ( (This)->lpVtbl -> get_ServiceListenAddress(This) ) 

#define IFabricCodePackageActivationContext6_get_ServicePublishAddress(This)	\
    ( (This)->lpVtbl -> get_ServicePublishAddress(This) ) 


#define IFabricCodePackageActivationContext6_GetDirectory(This,logicalDirectoryName,directoryPath)	\
    ( (This)->lpVtbl -> GetDirectory(This,logicalDirectoryName,directoryPath) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivationContext6_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackage_INTERFACE_DEFINED__
#define __IFabricCodePackage_INTERFACE_DEFINED__

/* interface IFabricCodePackage */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("20792b45-4d13-41a4-af13-346e529f00c5")
    IFabricCodePackage : public IUnknown
    {
    public:
        virtual const FABRIC_CODE_PACKAGE_DESCRIPTION *STDMETHODCALLTYPE get_Description( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_Path( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackage * This);
        
        const FABRIC_CODE_PACKAGE_DESCRIPTION *( STDMETHODCALLTYPE *get_Description )( 
            IFabricCodePackage * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_Path )( 
            IFabricCodePackage * This);
        
        END_INTERFACE
    } IFabricCodePackageVtbl;

    interface IFabricCodePackage
    {
        CONST_VTBL struct IFabricCodePackageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackage_get_Description(This)	\
    ( (This)->lpVtbl -> get_Description(This) ) 

#define IFabricCodePackage_get_Path(This)	\
    ( (This)->lpVtbl -> get_Path(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackage_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackage2_INTERFACE_DEFINED__
#define __IFabricCodePackage2_INTERFACE_DEFINED__

/* interface IFabricCodePackage2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackage2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cdf0a4e6-ad80-4cd6-b67e-e4c002428600")
    IFabricCodePackage2 : public IFabricCodePackage
    {
    public:
        virtual const FABRIC_RUNAS_POLICY_DESCRIPTION *STDMETHODCALLTYPE get_SetupEntryPointRunAsPolicy( void) = 0;
        
        virtual const FABRIC_RUNAS_POLICY_DESCRIPTION *STDMETHODCALLTYPE get_EntryPointRunAsPolicy( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackage2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackage2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackage2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackage2 * This);
        
        const FABRIC_CODE_PACKAGE_DESCRIPTION *( STDMETHODCALLTYPE *get_Description )( 
            IFabricCodePackage2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_Path )( 
            IFabricCodePackage2 * This);
        
        const FABRIC_RUNAS_POLICY_DESCRIPTION *( STDMETHODCALLTYPE *get_SetupEntryPointRunAsPolicy )( 
            IFabricCodePackage2 * This);
        
        const FABRIC_RUNAS_POLICY_DESCRIPTION *( STDMETHODCALLTYPE *get_EntryPointRunAsPolicy )( 
            IFabricCodePackage2 * This);
        
        END_INTERFACE
    } IFabricCodePackage2Vtbl;

    interface IFabricCodePackage2
    {
        CONST_VTBL struct IFabricCodePackage2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackage2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackage2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackage2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackage2_get_Description(This)	\
    ( (This)->lpVtbl -> get_Description(This) ) 

#define IFabricCodePackage2_get_Path(This)	\
    ( (This)->lpVtbl -> get_Path(This) ) 


#define IFabricCodePackage2_get_SetupEntryPointRunAsPolicy(This)	\
    ( (This)->lpVtbl -> get_SetupEntryPointRunAsPolicy(This) ) 

#define IFabricCodePackage2_get_EntryPointRunAsPolicy(This)	\
    ( (This)->lpVtbl -> get_EntryPointRunAsPolicy(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackage2_INTERFACE_DEFINED__ */


#ifndef __IFabricConfigurationPackage_INTERFACE_DEFINED__
#define __IFabricConfigurationPackage_INTERFACE_DEFINED__

/* interface IFabricConfigurationPackage */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricConfigurationPackage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ac4c3bfa-2563-46b7-a71d-2dca7b0a8f4d")
    IFabricConfigurationPackage : public IUnknown
    {
    public:
        virtual const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION *STDMETHODCALLTYPE get_Description( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_Path( void) = 0;
        
        virtual const FABRIC_CONFIGURATION_SETTINGS *STDMETHODCALLTYPE get_Settings( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSection( 
            /* [in] */ LPCWSTR sectionName,
            /* [retval][out] */ const FABRIC_CONFIGURATION_SECTION **bufferedValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterName,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ LPCWSTR *bufferedValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DecryptValue( 
            /* [in] */ LPCWSTR encryptedValue,
            /* [retval][out] */ IFabricStringResult **decryptedValue) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricConfigurationPackageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricConfigurationPackage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricConfigurationPackage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricConfigurationPackage * This);
        
        const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION *( STDMETHODCALLTYPE *get_Description )( 
            IFabricConfigurationPackage * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_Path )( 
            IFabricConfigurationPackage * This);
        
        const FABRIC_CONFIGURATION_SETTINGS *( STDMETHODCALLTYPE *get_Settings )( 
            IFabricConfigurationPackage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSection )( 
            IFabricConfigurationPackage * This,
            /* [in] */ LPCWSTR sectionName,
            /* [retval][out] */ const FABRIC_CONFIGURATION_SECTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            IFabricConfigurationPackage * This,
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterName,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ LPCWSTR *bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *DecryptValue )( 
            IFabricConfigurationPackage * This,
            /* [in] */ LPCWSTR encryptedValue,
            /* [retval][out] */ IFabricStringResult **decryptedValue);
        
        END_INTERFACE
    } IFabricConfigurationPackageVtbl;

    interface IFabricConfigurationPackage
    {
        CONST_VTBL struct IFabricConfigurationPackageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricConfigurationPackage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricConfigurationPackage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricConfigurationPackage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricConfigurationPackage_get_Description(This)	\
    ( (This)->lpVtbl -> get_Description(This) ) 

#define IFabricConfigurationPackage_get_Path(This)	\
    ( (This)->lpVtbl -> get_Path(This) ) 

#define IFabricConfigurationPackage_get_Settings(This)	\
    ( (This)->lpVtbl -> get_Settings(This) ) 

#define IFabricConfigurationPackage_GetSection(This,sectionName,bufferedValue)	\
    ( (This)->lpVtbl -> GetSection(This,sectionName,bufferedValue) ) 

#define IFabricConfigurationPackage_GetValue(This,sectionName,parameterName,isEncrypted,bufferedValue)	\
    ( (This)->lpVtbl -> GetValue(This,sectionName,parameterName,isEncrypted,bufferedValue) ) 

#define IFabricConfigurationPackage_DecryptValue(This,encryptedValue,decryptedValue)	\
    ( (This)->lpVtbl -> DecryptValue(This,encryptedValue,decryptedValue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricConfigurationPackage_INTERFACE_DEFINED__ */


#ifndef __IFabricConfigurationPackage2_INTERFACE_DEFINED__
#define __IFabricConfigurationPackage2_INTERFACE_DEFINED__

/* interface IFabricConfigurationPackage2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricConfigurationPackage2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d3161f31-708a-4f83-91ff-f2af15f74a2f")
    IFabricConfigurationPackage2 : public IFabricConfigurationPackage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetValues( 
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterPrefix,
            /* [retval][out] */ FABRIC_CONFIGURATION_PARAMETER_LIST **bufferedValue) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricConfigurationPackage2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricConfigurationPackage2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricConfigurationPackage2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricConfigurationPackage2 * This);
        
        const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION *( STDMETHODCALLTYPE *get_Description )( 
            IFabricConfigurationPackage2 * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_Path )( 
            IFabricConfigurationPackage2 * This);
        
        const FABRIC_CONFIGURATION_SETTINGS *( STDMETHODCALLTYPE *get_Settings )( 
            IFabricConfigurationPackage2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSection )( 
            IFabricConfigurationPackage2 * This,
            /* [in] */ LPCWSTR sectionName,
            /* [retval][out] */ const FABRIC_CONFIGURATION_SECTION **bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            IFabricConfigurationPackage2 * This,
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterName,
            /* [out] */ BOOLEAN *isEncrypted,
            /* [retval][out] */ LPCWSTR *bufferedValue);
        
        HRESULT ( STDMETHODCALLTYPE *DecryptValue )( 
            IFabricConfigurationPackage2 * This,
            /* [in] */ LPCWSTR encryptedValue,
            /* [retval][out] */ IFabricStringResult **decryptedValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetValues )( 
            IFabricConfigurationPackage2 * This,
            /* [in] */ LPCWSTR sectionName,
            /* [in] */ LPCWSTR parameterPrefix,
            /* [retval][out] */ FABRIC_CONFIGURATION_PARAMETER_LIST **bufferedValue);
        
        END_INTERFACE
    } IFabricConfigurationPackage2Vtbl;

    interface IFabricConfigurationPackage2
    {
        CONST_VTBL struct IFabricConfigurationPackage2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricConfigurationPackage2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricConfigurationPackage2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricConfigurationPackage2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricConfigurationPackage2_get_Description(This)	\
    ( (This)->lpVtbl -> get_Description(This) ) 

#define IFabricConfigurationPackage2_get_Path(This)	\
    ( (This)->lpVtbl -> get_Path(This) ) 

#define IFabricConfigurationPackage2_get_Settings(This)	\
    ( (This)->lpVtbl -> get_Settings(This) ) 

#define IFabricConfigurationPackage2_GetSection(This,sectionName,bufferedValue)	\
    ( (This)->lpVtbl -> GetSection(This,sectionName,bufferedValue) ) 

#define IFabricConfigurationPackage2_GetValue(This,sectionName,parameterName,isEncrypted,bufferedValue)	\
    ( (This)->lpVtbl -> GetValue(This,sectionName,parameterName,isEncrypted,bufferedValue) ) 

#define IFabricConfigurationPackage2_DecryptValue(This,encryptedValue,decryptedValue)	\
    ( (This)->lpVtbl -> DecryptValue(This,encryptedValue,decryptedValue) ) 


#define IFabricConfigurationPackage2_GetValues(This,sectionName,parameterPrefix,bufferedValue)	\
    ( (This)->lpVtbl -> GetValues(This,sectionName,parameterPrefix,bufferedValue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricConfigurationPackage2_INTERFACE_DEFINED__ */


#ifndef __IFabricDataPackage_INTERFACE_DEFINED__
#define __IFabricDataPackage_INTERFACE_DEFINED__

/* interface IFabricDataPackage */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricDataPackage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aa67de09-3657-435f-a2f6-b3a17a0a4371")
    IFabricDataPackage : public IUnknown
    {
    public:
        virtual const FABRIC_DATA_PACKAGE_DESCRIPTION *STDMETHODCALLTYPE get_Description( void) = 0;
        
        virtual LPCWSTR STDMETHODCALLTYPE get_Path( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricDataPackageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricDataPackage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricDataPackage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricDataPackage * This);
        
        const FABRIC_DATA_PACKAGE_DESCRIPTION *( STDMETHODCALLTYPE *get_Description )( 
            IFabricDataPackage * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_Path )( 
            IFabricDataPackage * This);
        
        END_INTERFACE
    } IFabricDataPackageVtbl;

    interface IFabricDataPackage
    {
        CONST_VTBL struct IFabricDataPackageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricDataPackage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricDataPackage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricDataPackage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricDataPackage_get_Description(This)	\
    ( (This)->lpVtbl -> get_Description(This) ) 

#define IFabricDataPackage_get_Path(This)	\
    ( (This)->lpVtbl -> get_Path(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricDataPackage_INTERFACE_DEFINED__ */


#ifndef __IFabricConfigurationPackageChangeHandler_INTERFACE_DEFINED__
#define __IFabricConfigurationPackageChangeHandler_INTERFACE_DEFINED__

/* interface IFabricConfigurationPackageChangeHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricConfigurationPackageChangeHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c3954d48-b5ee-4ff4-9bc0-c30f6d0d3a85")
    IFabricConfigurationPackageChangeHandler : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnPackageAdded( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricConfigurationPackage *configPackage) = 0;
        
        virtual void STDMETHODCALLTYPE OnPackageRemoved( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricConfigurationPackage *configPackage) = 0;
        
        virtual void STDMETHODCALLTYPE OnPackageModified( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricConfigurationPackage *previousConfigPackage,
            /* [in] */ IFabricConfigurationPackage *configPackage) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricConfigurationPackageChangeHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricConfigurationPackageChangeHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricConfigurationPackageChangeHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricConfigurationPackageChangeHandler * This);
        
        void ( STDMETHODCALLTYPE *OnPackageAdded )( 
            IFabricConfigurationPackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricConfigurationPackage *configPackage);
        
        void ( STDMETHODCALLTYPE *OnPackageRemoved )( 
            IFabricConfigurationPackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricConfigurationPackage *configPackage);
        
        void ( STDMETHODCALLTYPE *OnPackageModified )( 
            IFabricConfigurationPackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricConfigurationPackage *previousConfigPackage,
            /* [in] */ IFabricConfigurationPackage *configPackage);
        
        END_INTERFACE
    } IFabricConfigurationPackageChangeHandlerVtbl;

    interface IFabricConfigurationPackageChangeHandler
    {
        CONST_VTBL struct IFabricConfigurationPackageChangeHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricConfigurationPackageChangeHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricConfigurationPackageChangeHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricConfigurationPackageChangeHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricConfigurationPackageChangeHandler_OnPackageAdded(This,source,configPackage)	\
    ( (This)->lpVtbl -> OnPackageAdded(This,source,configPackage) ) 

#define IFabricConfigurationPackageChangeHandler_OnPackageRemoved(This,source,configPackage)	\
    ( (This)->lpVtbl -> OnPackageRemoved(This,source,configPackage) ) 

#define IFabricConfigurationPackageChangeHandler_OnPackageModified(This,source,previousConfigPackage,configPackage)	\
    ( (This)->lpVtbl -> OnPackageModified(This,source,previousConfigPackage,configPackage) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricConfigurationPackageChangeHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricDataPackageChangeHandler_INTERFACE_DEFINED__
#define __IFabricDataPackageChangeHandler_INTERFACE_DEFINED__

/* interface IFabricDataPackageChangeHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricDataPackageChangeHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8d0a726f-bd17-4b32-807b-be2a8024b2e0")
    IFabricDataPackageChangeHandler : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnPackageAdded( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricDataPackage *dataPackage) = 0;
        
        virtual void STDMETHODCALLTYPE OnPackageRemoved( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricDataPackage *dataPackage) = 0;
        
        virtual void STDMETHODCALLTYPE OnPackageModified( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricDataPackage *previousDataPackage,
            /* [in] */ IFabricDataPackage *dataPackage) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricDataPackageChangeHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricDataPackageChangeHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricDataPackageChangeHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricDataPackageChangeHandler * This);
        
        void ( STDMETHODCALLTYPE *OnPackageAdded )( 
            IFabricDataPackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricDataPackage *dataPackage);
        
        void ( STDMETHODCALLTYPE *OnPackageRemoved )( 
            IFabricDataPackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricDataPackage *dataPackage);
        
        void ( STDMETHODCALLTYPE *OnPackageModified )( 
            IFabricDataPackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricDataPackage *previousDataPackage,
            /* [in] */ IFabricDataPackage *dataPackage);
        
        END_INTERFACE
    } IFabricDataPackageChangeHandlerVtbl;

    interface IFabricDataPackageChangeHandler
    {
        CONST_VTBL struct IFabricDataPackageChangeHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricDataPackageChangeHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricDataPackageChangeHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricDataPackageChangeHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricDataPackageChangeHandler_OnPackageAdded(This,source,dataPackage)	\
    ( (This)->lpVtbl -> OnPackageAdded(This,source,dataPackage) ) 

#define IFabricDataPackageChangeHandler_OnPackageRemoved(This,source,dataPackage)	\
    ( (This)->lpVtbl -> OnPackageRemoved(This,source,dataPackage) ) 

#define IFabricDataPackageChangeHandler_OnPackageModified(This,source,previousDataPackage,dataPackage)	\
    ( (This)->lpVtbl -> OnPackageModified(This,source,previousDataPackage,dataPackage) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricDataPackageChangeHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricTransactionBase_INTERFACE_DEFINED__
#define __IFabricTransactionBase_INTERFACE_DEFINED__

/* interface IFabricTransactionBase */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransactionBase;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("32d656a1-7ad5-47b8-bd66-a2e302626b7e")
    IFabricTransactionBase : public IUnknown
    {
    public:
        virtual const FABRIC_TRANSACTION_ID *STDMETHODCALLTYPE get_Id( void) = 0;
        
        virtual FABRIC_TRANSACTION_ISOLATION_LEVEL STDMETHODCALLTYPE get_IsolationLevel( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransactionBaseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransactionBase * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransactionBase * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransactionBase * This);
        
        const FABRIC_TRANSACTION_ID *( STDMETHODCALLTYPE *get_Id )( 
            IFabricTransactionBase * This);
        
        FABRIC_TRANSACTION_ISOLATION_LEVEL ( STDMETHODCALLTYPE *get_IsolationLevel )( 
            IFabricTransactionBase * This);
        
        END_INTERFACE
    } IFabricTransactionBaseVtbl;

    interface IFabricTransactionBase
    {
        CONST_VTBL struct IFabricTransactionBaseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransactionBase_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransactionBase_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransactionBase_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransactionBase_get_Id(This)	\
    ( (This)->lpVtbl -> get_Id(This) ) 

#define IFabricTransactionBase_get_IsolationLevel(This)	\
    ( (This)->lpVtbl -> get_IsolationLevel(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransactionBase_INTERFACE_DEFINED__ */


#ifndef __IFabricTransaction_INTERFACE_DEFINED__
#define __IFabricTransaction_INTERFACE_DEFINED__

/* interface IFabricTransaction */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransaction;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19ee48b4-6d4d-470b-ac1e-2d3996a173c8")
    IFabricTransaction : public IFabricTransactionBase
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginCommit( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCommit( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber) = 0;
        
        virtual void STDMETHODCALLTYPE Rollback( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransactionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransaction * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransaction * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransaction * This);
        
        const FABRIC_TRANSACTION_ID *( STDMETHODCALLTYPE *get_Id )( 
            IFabricTransaction * This);
        
        FABRIC_TRANSACTION_ISOLATION_LEVEL ( STDMETHODCALLTYPE *get_IsolationLevel )( 
            IFabricTransaction * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCommit )( 
            IFabricTransaction * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCommit )( 
            IFabricTransaction * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *commitSequenceNumber);
        
        void ( STDMETHODCALLTYPE *Rollback )( 
            IFabricTransaction * This);
        
        END_INTERFACE
    } IFabricTransactionVtbl;

    interface IFabricTransaction
    {
        CONST_VTBL struct IFabricTransactionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransaction_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransaction_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransaction_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransaction_get_Id(This)	\
    ( (This)->lpVtbl -> get_Id(This) ) 

#define IFabricTransaction_get_IsolationLevel(This)	\
    ( (This)->lpVtbl -> get_IsolationLevel(This) ) 


#define IFabricTransaction_BeginCommit(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCommit(This,timeoutMilliseconds,callback,context) ) 

#define IFabricTransaction_EndCommit(This,context,commitSequenceNumber)	\
    ( (This)->lpVtbl -> EndCommit(This,context,commitSequenceNumber) ) 

#define IFabricTransaction_Rollback(This)	\
    ( (This)->lpVtbl -> Rollback(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransaction_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplica_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplica */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplica;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97da35c4-38ed-4a2a-8f37-fbeb56382235")
    IFabricKeyValueStoreReplica : public IFabricStatefulServiceReplica
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCurrentEpoch( 
            /* [out] */ FABRIC_EPOCH *currentEpoch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings( 
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTransaction( 
            /* [retval][out] */ IFabricTransaction **transaction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMetadata( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Contains( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enumerate( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateByKey( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateMetadata( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplicaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplica * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplica * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricKeyValueStoreReplica * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentEpoch )( 
            IFabricKeyValueStoreReplica * This,
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction )( 
            IFabricKeyValueStoreReplica * This,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Update )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *Contains )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadata )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreReplica * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplicaVtbl;

    interface IFabricKeyValueStoreReplica
    {
        CONST_VTBL struct IFabricKeyValueStoreReplicaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplica_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplica_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplica_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplica_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricKeyValueStoreReplica_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricKeyValueStoreReplica_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricKeyValueStoreReplica_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricKeyValueStoreReplica_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricKeyValueStoreReplica_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricKeyValueStoreReplica_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 


#define IFabricKeyValueStoreReplica_GetCurrentEpoch(This,currentEpoch)	\
    ( (This)->lpVtbl -> GetCurrentEpoch(This,currentEpoch) ) 

#define IFabricKeyValueStoreReplica_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#define IFabricKeyValueStoreReplica_CreateTransaction(This,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction(This,transaction) ) 

#define IFabricKeyValueStoreReplica_Add(This,transaction,key,valueSizeInBytes,value)	\
    ( (This)->lpVtbl -> Add(This,transaction,key,valueSizeInBytes,value) ) 

#define IFabricKeyValueStoreReplica_Remove(This,transaction,key,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Remove(This,transaction,key,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica_Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica_Get(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Get(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica_GetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> GetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica_Contains(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Contains(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica_Enumerate(This,transaction,result)	\
    ( (This)->lpVtbl -> Enumerate(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica_EnumerateByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,transaction,keyPrefix,result) ) 

#define IFabricKeyValueStoreReplica_EnumerateMetadata(This,transaction,result)	\
    ( (This)->lpVtbl -> EnumerateMetadata(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica_EnumerateMetadataByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,transaction,keyPrefix,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplica_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica2_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplica2_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplica2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplica2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fef805b2-5aca-4caa-9c51-fb3bd577a792")
    IFabricKeyValueStoreReplica2 : public IFabricKeyValueStoreReplica
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Backup( 
            /* [in] */ LPCWSTR backupDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Restore( 
            /* [in] */ LPCWSTR backupDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTransaction2( 
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS *settings,
            /* [retval][out] */ IFabricTransaction **transaction) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplica2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplica2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplica2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricKeyValueStoreReplica2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentEpoch )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Update )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *Contains )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadata )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction2 )( 
            IFabricKeyValueStoreReplica2 * This,
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS *settings,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplica2Vtbl;

    interface IFabricKeyValueStoreReplica2
    {
        CONST_VTBL struct IFabricKeyValueStoreReplica2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplica2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplica2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplica2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplica2_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricKeyValueStoreReplica2_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricKeyValueStoreReplica2_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricKeyValueStoreReplica2_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricKeyValueStoreReplica2_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricKeyValueStoreReplica2_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricKeyValueStoreReplica2_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 


#define IFabricKeyValueStoreReplica2_GetCurrentEpoch(This,currentEpoch)	\
    ( (This)->lpVtbl -> GetCurrentEpoch(This,currentEpoch) ) 

#define IFabricKeyValueStoreReplica2_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#define IFabricKeyValueStoreReplica2_CreateTransaction(This,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction(This,transaction) ) 

#define IFabricKeyValueStoreReplica2_Add(This,transaction,key,valueSizeInBytes,value)	\
    ( (This)->lpVtbl -> Add(This,transaction,key,valueSizeInBytes,value) ) 

#define IFabricKeyValueStoreReplica2_Remove(This,transaction,key,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Remove(This,transaction,key,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica2_Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica2_Get(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Get(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica2_GetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> GetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica2_Contains(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Contains(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica2_Enumerate(This,transaction,result)	\
    ( (This)->lpVtbl -> Enumerate(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica2_EnumerateByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,transaction,keyPrefix,result) ) 

#define IFabricKeyValueStoreReplica2_EnumerateMetadata(This,transaction,result)	\
    ( (This)->lpVtbl -> EnumerateMetadata(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica2_EnumerateMetadataByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,transaction,keyPrefix,result) ) 


#define IFabricKeyValueStoreReplica2_Backup(This,backupDirectory)	\
    ( (This)->lpVtbl -> Backup(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica2_Restore(This,backupDirectory)	\
    ( (This)->lpVtbl -> Restore(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica2_CreateTransaction2(This,settings,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction2(This,settings,transaction) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplica2_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica3_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplica3_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplica3 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplica3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c1297172-a8aa-4096-bdcc-1ece0c5d8c8f")
    IFabricKeyValueStoreReplica3 : public IFabricKeyValueStoreReplica2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginBackup( 
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
            /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndBackup( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplica3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplica3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplica3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricKeyValueStoreReplica3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentEpoch )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Update )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *Contains )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadata )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction2 )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS *settings,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *BeginBackup )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
            /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndBackup )( 
            IFabricKeyValueStoreReplica3 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplica3Vtbl;

    interface IFabricKeyValueStoreReplica3
    {
        CONST_VTBL struct IFabricKeyValueStoreReplica3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplica3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplica3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplica3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplica3_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricKeyValueStoreReplica3_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricKeyValueStoreReplica3_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricKeyValueStoreReplica3_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricKeyValueStoreReplica3_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricKeyValueStoreReplica3_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricKeyValueStoreReplica3_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 


#define IFabricKeyValueStoreReplica3_GetCurrentEpoch(This,currentEpoch)	\
    ( (This)->lpVtbl -> GetCurrentEpoch(This,currentEpoch) ) 

#define IFabricKeyValueStoreReplica3_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#define IFabricKeyValueStoreReplica3_CreateTransaction(This,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction(This,transaction) ) 

#define IFabricKeyValueStoreReplica3_Add(This,transaction,key,valueSizeInBytes,value)	\
    ( (This)->lpVtbl -> Add(This,transaction,key,valueSizeInBytes,value) ) 

#define IFabricKeyValueStoreReplica3_Remove(This,transaction,key,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Remove(This,transaction,key,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica3_Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica3_Get(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Get(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica3_GetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> GetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica3_Contains(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Contains(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica3_Enumerate(This,transaction,result)	\
    ( (This)->lpVtbl -> Enumerate(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica3_EnumerateByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,transaction,keyPrefix,result) ) 

#define IFabricKeyValueStoreReplica3_EnumerateMetadata(This,transaction,result)	\
    ( (This)->lpVtbl -> EnumerateMetadata(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica3_EnumerateMetadataByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,transaction,keyPrefix,result) ) 


#define IFabricKeyValueStoreReplica3_Backup(This,backupDirectory)	\
    ( (This)->lpVtbl -> Backup(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica3_Restore(This,backupDirectory)	\
    ( (This)->lpVtbl -> Restore(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica3_CreateTransaction2(This,settings,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction2(This,settings,transaction) ) 


#define IFabricKeyValueStoreReplica3_BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context)	\
    ( (This)->lpVtbl -> BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context) ) 

#define IFabricKeyValueStoreReplica3_EndBackup(This,context)	\
    ( (This)->lpVtbl -> EndBackup(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplica3_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemEnumerator_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreItemEnumerator_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreItemEnumerator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreItemEnumerator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c202788f-54d3-44a6-8f3c-b4bbfcdb95d2")
    IFabricKeyValueStoreItemEnumerator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
        virtual IFabricKeyValueStoreItemResult *STDMETHODCALLTYPE get_Current( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreItemEnumeratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreItemEnumerator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreItemEnumerator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreItemEnumerator * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFabricKeyValueStoreItemEnumerator * This);
        
        IFabricKeyValueStoreItemResult *( STDMETHODCALLTYPE *get_Current )( 
            IFabricKeyValueStoreItemEnumerator * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreItemEnumeratorVtbl;

    interface IFabricKeyValueStoreItemEnumerator
    {
        CONST_VTBL struct IFabricKeyValueStoreItemEnumeratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreItemEnumerator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreItemEnumerator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreItemEnumerator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreItemEnumerator_MoveNext(This)	\
    ( (This)->lpVtbl -> MoveNext(This) ) 

#define IFabricKeyValueStoreItemEnumerator_get_Current(This)	\
    ( (This)->lpVtbl -> get_Current(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreItemEnumerator_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataEnumerator_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreItemMetadataEnumerator_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreItemMetadataEnumerator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreItemMetadataEnumerator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0bc06aee-fffa-4450-9099-116a5f0e0b53")
    IFabricKeyValueStoreItemMetadataEnumerator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
        virtual IFabricKeyValueStoreItemMetadataResult *STDMETHODCALLTYPE get_Current( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreItemMetadataEnumeratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreItemMetadataEnumerator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreItemMetadataEnumerator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreItemMetadataEnumerator * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFabricKeyValueStoreItemMetadataEnumerator * This);
        
        IFabricKeyValueStoreItemMetadataResult *( STDMETHODCALLTYPE *get_Current )( 
            IFabricKeyValueStoreItemMetadataEnumerator * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreItemMetadataEnumeratorVtbl;

    interface IFabricKeyValueStoreItemMetadataEnumerator
    {
        CONST_VTBL struct IFabricKeyValueStoreItemMetadataEnumeratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreItemMetadataEnumerator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreItemMetadataEnumerator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreItemMetadataEnumerator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreItemMetadataEnumerator_MoveNext(This)	\
    ( (This)->lpVtbl -> MoveNext(This) ) 

#define IFabricKeyValueStoreItemMetadataEnumerator_get_Current(This)	\
    ( (This)->lpVtbl -> get_Current(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreItemMetadataEnumerator_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemResult_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreItemResult_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreItemResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreItemResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c1f1c89d-b0b8-44dc-bc97-6c074c1a805e")
    IFabricKeyValueStoreItemResult : public IUnknown
    {
    public:
        virtual const FABRIC_KEY_VALUE_STORE_ITEM *STDMETHODCALLTYPE get_Item( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreItemResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreItemResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreItemResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreItemResult * This);
        
        const FABRIC_KEY_VALUE_STORE_ITEM *( STDMETHODCALLTYPE *get_Item )( 
            IFabricKeyValueStoreItemResult * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreItemResultVtbl;

    interface IFabricKeyValueStoreItemResult
    {
        CONST_VTBL struct IFabricKeyValueStoreItemResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreItemResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreItemResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreItemResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreItemResult_get_Item(This)	\
    ( (This)->lpVtbl -> get_Item(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreItemResult_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataResult_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreItemMetadataResult_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreItemMetadataResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreItemMetadataResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17c483a1-69e6-4bdc-a058-54fd4a1839fd")
    IFabricKeyValueStoreItemMetadataResult : public IUnknown
    {
    public:
        virtual const FABRIC_KEY_VALUE_STORE_ITEM_METADATA *STDMETHODCALLTYPE get_Metadata( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreItemMetadataResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreItemMetadataResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreItemMetadataResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreItemMetadataResult * This);
        
        const FABRIC_KEY_VALUE_STORE_ITEM_METADATA *( STDMETHODCALLTYPE *get_Metadata )( 
            IFabricKeyValueStoreItemMetadataResult * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreItemMetadataResultVtbl;

    interface IFabricKeyValueStoreItemMetadataResult
    {
        CONST_VTBL struct IFabricKeyValueStoreItemMetadataResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreItemMetadataResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreItemMetadataResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreItemMetadataResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreItemMetadataResult_get_Metadata(This)	\
    ( (This)->lpVtbl -> get_Metadata(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreItemMetadataResult_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotification_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreNotification_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreNotification */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cb660aa6-c51e-4f05-9526-93982b550e8f")
    IFabricKeyValueStoreNotification : public IFabricKeyValueStoreItemResult
    {
    public:
        virtual BOOLEAN STDMETHODCALLTYPE IsDelete( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreNotification * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreNotification * This);
        
        const FABRIC_KEY_VALUE_STORE_ITEM *( STDMETHODCALLTYPE *get_Item )( 
            IFabricKeyValueStoreNotification * This);
        
        BOOLEAN ( STDMETHODCALLTYPE *IsDelete )( 
            IFabricKeyValueStoreNotification * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreNotificationVtbl;

    interface IFabricKeyValueStoreNotification
    {
        CONST_VTBL struct IFabricKeyValueStoreNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreNotification_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreNotification_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreNotification_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreNotification_get_Item(This)	\
    ( (This)->lpVtbl -> get_Item(This) ) 


#define IFabricKeyValueStoreNotification_IsDelete(This)	\
    ( (This)->lpVtbl -> IsDelete(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreNotification_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotificationEnumerator_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreNotificationEnumerator_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreNotificationEnumerator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreNotificationEnumerator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ef25bc08-be76-43c7-adad-20f01fba3399")
    IFabricKeyValueStoreNotificationEnumerator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
        virtual IFabricKeyValueStoreNotification *STDMETHODCALLTYPE get_Current( void) = 0;
        
        virtual void STDMETHODCALLTYPE Reset( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreNotificationEnumeratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreNotificationEnumerator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreNotificationEnumerator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreNotificationEnumerator * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFabricKeyValueStoreNotificationEnumerator * This);
        
        IFabricKeyValueStoreNotification *( STDMETHODCALLTYPE *get_Current )( 
            IFabricKeyValueStoreNotificationEnumerator * This);
        
        void ( STDMETHODCALLTYPE *Reset )( 
            IFabricKeyValueStoreNotificationEnumerator * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreNotificationEnumeratorVtbl;

    interface IFabricKeyValueStoreNotificationEnumerator
    {
        CONST_VTBL struct IFabricKeyValueStoreNotificationEnumeratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreNotificationEnumerator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreNotificationEnumerator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreNotificationEnumerator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreNotificationEnumerator_MoveNext(This)	\
    ( (This)->lpVtbl -> MoveNext(This) ) 

#define IFabricKeyValueStoreNotificationEnumerator_get_Current(This)	\
    ( (This)->lpVtbl -> get_Current(This) ) 

#define IFabricKeyValueStoreNotificationEnumerator_Reset(This)	\
    ( (This)->lpVtbl -> Reset(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreNotificationEnumerator_INTERFACE_DEFINED__ */


#ifndef __IFabricNodeContextResult_INTERFACE_DEFINED__
#define __IFabricNodeContextResult_INTERFACE_DEFINED__

/* interface IFabricNodeContextResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricNodeContextResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0952f885-6f5a-4ed3-abe4-90c403d1e3ce")
    IFabricNodeContextResult : public IUnknown
    {
    public:
        virtual const FABRIC_NODE_CONTEXT *STDMETHODCALLTYPE get_NodeContext( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricNodeContextResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricNodeContextResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricNodeContextResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricNodeContextResult * This);
        
        const FABRIC_NODE_CONTEXT *( STDMETHODCALLTYPE *get_NodeContext )( 
            IFabricNodeContextResult * This);
        
        END_INTERFACE
    } IFabricNodeContextResultVtbl;

    interface IFabricNodeContextResult
    {
        CONST_VTBL struct IFabricNodeContextResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricNodeContextResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricNodeContextResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricNodeContextResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricNodeContextResult_get_NodeContext(This)	\
    ( (This)->lpVtbl -> get_NodeContext(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricNodeContextResult_INTERFACE_DEFINED__ */


#ifndef __IFabricNodeContextResult2_INTERFACE_DEFINED__
#define __IFabricNodeContextResult2_INTERFACE_DEFINED__

/* interface IFabricNodeContextResult2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricNodeContextResult2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("472bf2e1-d617-4b5c-a91d-fabed9ff3550")
    IFabricNodeContextResult2 : public IFabricNodeContextResult
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDirectory( 
            /* [in] */ LPCWSTR logicalDirectoryName,
            /* [retval][out] */ IFabricStringResult **directoryPath) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricNodeContextResult2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricNodeContextResult2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricNodeContextResult2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricNodeContextResult2 * This);
        
        const FABRIC_NODE_CONTEXT *( STDMETHODCALLTYPE *get_NodeContext )( 
            IFabricNodeContextResult2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDirectory )( 
            IFabricNodeContextResult2 * This,
            /* [in] */ LPCWSTR logicalDirectoryName,
            /* [retval][out] */ IFabricStringResult **directoryPath);
        
        END_INTERFACE
    } IFabricNodeContextResult2Vtbl;

    interface IFabricNodeContextResult2
    {
        CONST_VTBL struct IFabricNodeContextResult2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricNodeContextResult2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricNodeContextResult2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricNodeContextResult2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricNodeContextResult2_get_NodeContext(This)	\
    ( (This)->lpVtbl -> get_NodeContext(This) ) 


#define IFabricNodeContextResult2_GetDirectory(This,logicalDirectoryName,directoryPath)	\
    ( (This)->lpVtbl -> GetDirectory(This,logicalDirectoryName,directoryPath) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricNodeContextResult2_INTERFACE_DEFINED__ */


#ifndef __IFabricReplicatorSettingsResult_INTERFACE_DEFINED__
#define __IFabricReplicatorSettingsResult_INTERFACE_DEFINED__

/* interface IFabricReplicatorSettingsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricReplicatorSettingsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("718954F3-DC1E-4060-9806-0CBF36F71051")
    IFabricReplicatorSettingsResult : public IUnknown
    {
    public:
        virtual const FABRIC_REPLICATOR_SETTINGS *STDMETHODCALLTYPE get_ReplicatorSettings( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricReplicatorSettingsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricReplicatorSettingsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricReplicatorSettingsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricReplicatorSettingsResult * This);
        
        const FABRIC_REPLICATOR_SETTINGS *( STDMETHODCALLTYPE *get_ReplicatorSettings )( 
            IFabricReplicatorSettingsResult * This);
        
        END_INTERFACE
    } IFabricReplicatorSettingsResultVtbl;

    interface IFabricReplicatorSettingsResult
    {
        CONST_VTBL struct IFabricReplicatorSettingsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricReplicatorSettingsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricReplicatorSettingsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricReplicatorSettingsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricReplicatorSettingsResult_get_ReplicatorSettings(This)	\
    ( (This)->lpVtbl -> get_ReplicatorSettings(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricReplicatorSettingsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricEseLocalStoreSettingsResult_INTERFACE_DEFINED__
#define __IFabricEseLocalStoreSettingsResult_INTERFACE_DEFINED__

/* interface IFabricEseLocalStoreSettingsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricEseLocalStoreSettingsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AACE77AE-D8E1-4144-B1EE-5AC74FD54F65")
    IFabricEseLocalStoreSettingsResult : public IUnknown
    {
    public:
        virtual const FABRIC_ESE_LOCAL_STORE_SETTINGS *STDMETHODCALLTYPE get_Settings( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricEseLocalStoreSettingsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricEseLocalStoreSettingsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricEseLocalStoreSettingsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricEseLocalStoreSettingsResult * This);
        
        const FABRIC_ESE_LOCAL_STORE_SETTINGS *( STDMETHODCALLTYPE *get_Settings )( 
            IFabricEseLocalStoreSettingsResult * This);
        
        END_INTERFACE
    } IFabricEseLocalStoreSettingsResultVtbl;

    interface IFabricEseLocalStoreSettingsResult
    {
        CONST_VTBL struct IFabricEseLocalStoreSettingsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricEseLocalStoreSettingsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricEseLocalStoreSettingsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricEseLocalStoreSettingsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricEseLocalStoreSettingsResult_get_Settings(This)	\
    ( (This)->lpVtbl -> get_Settings(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricEseLocalStoreSettingsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricSecurityCredentialsResult_INTERFACE_DEFINED__
#define __IFabricSecurityCredentialsResult_INTERFACE_DEFINED__

/* interface IFabricSecurityCredentialsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricSecurityCredentialsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("049A111D-6A30-48E9-8F69-470760D3EFB9")
    IFabricSecurityCredentialsResult : public IUnknown
    {
    public:
        virtual const FABRIC_SECURITY_CREDENTIALS *STDMETHODCALLTYPE get_SecurityCredentials( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricSecurityCredentialsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricSecurityCredentialsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricSecurityCredentialsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricSecurityCredentialsResult * This);
        
        const FABRIC_SECURITY_CREDENTIALS *( STDMETHODCALLTYPE *get_SecurityCredentials )( 
            IFabricSecurityCredentialsResult * This);
        
        END_INTERFACE
    } IFabricSecurityCredentialsResultVtbl;

    interface IFabricSecurityCredentialsResult
    {
        CONST_VTBL struct IFabricSecurityCredentialsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricSecurityCredentialsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricSecurityCredentialsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricSecurityCredentialsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricSecurityCredentialsResult_get_SecurityCredentials(This)	\
    ( (This)->lpVtbl -> get_SecurityCredentials(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricSecurityCredentialsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageActivator_INTERFACE_DEFINED__
#define __IFabricCodePackageActivator_INTERFACE_DEFINED__

/* interface IFabricCodePackageActivator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageActivator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("70BE1B10-B259-46FC-B813-0B75720E7183")
    IFabricCodePackageActivator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginActivateCodePackage( 
            /* [in] */ FABRIC_STRING_LIST *codePackageNames,
            /* [in] */ FABRIC_STRING_MAP *environment,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndActivateCodePackage( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeactivateCodePackage( 
            /* [in] */ FABRIC_STRING_LIST *codePackageNames,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeactivateCodePackage( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbortCodePackage( 
            /* [in] */ FABRIC_STRING_LIST *codePackageNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterCodePackageEventHandler( 
            /* [in] */ IFabricCodePackageEventHandler *eventHandler,
            /* [retval][out] */ ULONGLONG *callbackHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterCodePackageEventHandler( 
            /* [in] */ ULONGLONG callbackHandle) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageActivatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageActivator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageActivator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageActivator * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginActivateCodePackage )( 
            IFabricCodePackageActivator * This,
            /* [in] */ FABRIC_STRING_LIST *codePackageNames,
            /* [in] */ FABRIC_STRING_MAP *environment,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndActivateCodePackage )( 
            IFabricCodePackageActivator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeactivateCodePackage )( 
            IFabricCodePackageActivator * This,
            /* [in] */ FABRIC_STRING_LIST *codePackageNames,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeactivateCodePackage )( 
            IFabricCodePackageActivator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *AbortCodePackage )( 
            IFabricCodePackageActivator * This,
            /* [in] */ FABRIC_STRING_LIST *codePackageNames);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterCodePackageEventHandler )( 
            IFabricCodePackageActivator * This,
            /* [in] */ IFabricCodePackageEventHandler *eventHandler,
            /* [retval][out] */ ULONGLONG *callbackHandle);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterCodePackageEventHandler )( 
            IFabricCodePackageActivator * This,
            /* [in] */ ULONGLONG callbackHandle);
        
        END_INTERFACE
    } IFabricCodePackageActivatorVtbl;

    interface IFabricCodePackageActivator
    {
        CONST_VTBL struct IFabricCodePackageActivatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageActivator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageActivator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageActivator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageActivator_BeginActivateCodePackage(This,codePackageNames,environment,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginActivateCodePackage(This,codePackageNames,environment,timeoutMilliseconds,callback,context) ) 

#define IFabricCodePackageActivator_EndActivateCodePackage(This,context)	\
    ( (This)->lpVtbl -> EndActivateCodePackage(This,context) ) 

#define IFabricCodePackageActivator_BeginDeactivateCodePackage(This,codePackageNames,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeactivateCodePackage(This,codePackageNames,timeoutMilliseconds,callback,context) ) 

#define IFabricCodePackageActivator_EndDeactivateCodePackage(This,context)	\
    ( (This)->lpVtbl -> EndDeactivateCodePackage(This,context) ) 

#define IFabricCodePackageActivator_AbortCodePackage(This,codePackageNames)	\
    ( (This)->lpVtbl -> AbortCodePackage(This,codePackageNames) ) 

#define IFabricCodePackageActivator_RegisterCodePackageEventHandler(This,eventHandler,callbackHandle)	\
    ( (This)->lpVtbl -> RegisterCodePackageEventHandler(This,eventHandler,callbackHandle) ) 

#define IFabricCodePackageActivator_UnregisterCodePackageEventHandler(This,callbackHandle)	\
    ( (This)->lpVtbl -> UnregisterCodePackageEventHandler(This,callbackHandle) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageActivator_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageEventHandler_INTERFACE_DEFINED__
#define __IFabricCodePackageEventHandler_INTERFACE_DEFINED__

/* interface IFabricCodePackageEventHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("899E0CA8-16DF-458E-8915-D0307B4AB101")
    IFabricCodePackageEventHandler : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnCodePackageEvent( 
            /* [in] */ IFabricCodePackageActivator *source,
            /* [in] */ const FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION *eventDesc) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageEventHandler * This);
        
        void ( STDMETHODCALLTYPE *OnCodePackageEvent )( 
            IFabricCodePackageEventHandler * This,
            /* [in] */ IFabricCodePackageActivator *source,
            /* [in] */ const FABRIC_CODE_PACKAGE_EVENT_DESCRIPTION *eventDesc);
        
        END_INTERFACE
    } IFabricCodePackageEventHandlerVtbl;

    interface IFabricCodePackageEventHandler
    {
        CONST_VTBL struct IFabricCodePackageEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageEventHandler_OnCodePackageEvent(This,source,eventDesc)	\
    ( (This)->lpVtbl -> OnCodePackageEvent(This,source,eventDesc) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageEventHandler_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricRuntime;

#ifdef __cplusplus

class DECLSPEC_UUID("cc53af8c-74cd-11df-ac3e-0024811e3892")
FabricRuntime;
#endif


#ifndef __FabricRuntimeModule_MODULE_DEFINED__
#define __FabricRuntimeModule_MODULE_DEFINED__


/* module FabricRuntimeModule */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT FabricBeginCreateRuntime( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in_opt IFabricProcessExitHandler *exitHandler,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

/* [entry] */ HRESULT FabricEndCreateRuntime( 
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricRuntime);

/* [entry] */ HRESULT FabricCreateRuntime( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricRuntime);

/* [entry] */ HRESULT FabricBeginGetActivationContext( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

/* [entry] */ HRESULT FabricEndGetActivationContext( 
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **activationContext);

/* [entry] */ HRESULT FabricGetActivationContext( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **activationContext);

/* [entry] */ HRESULT FabricCreateKeyValueStoreReplica( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
    /* [in] */ __RPC__in void *localStoreSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);

/* [entry] */ HRESULT FabricCreateKeyValueStoreReplica2( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
    /* [in] */ __RPC__in void *localStoreSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [in] */ FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE notificationMode,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);

/* [entry] */ HRESULT FabricCreateKeyValueStoreReplica3( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
    /* [in] */ __RPC__in void *localStoreSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [in] */ FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE notificationMode,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);

/* [entry] */ HRESULT FabricCreateKeyValueStoreReplica4( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in FABRIC_URI serviceName,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
    /* [in] */ __RPC__in void *localStoreSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [in] */ FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE notificationMode,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);

/* [entry] */ HRESULT FabricCreateKeyValueStoreReplica5( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR storeName,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in FABRIC_URI serviceName,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ __RPC__in const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS *kvsSettings,
    /* [in] */ FABRIC_LOCAL_STORE_KIND localStoreKind,
    /* [in] */ __RPC__in void *localStoreSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);

/* [entry] */ HRESULT FabricBeginGetNodeContext( 
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

/* [entry] */ HRESULT FabricEndGetNodeContext( 
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **nodeContext);

/* [entry] */ HRESULT FabricGetNodeContext( 
    /* [retval][out] */ __RPC__deref_out_opt void **nodeContext);

/* [entry] */ HRESULT FabricLoadReplicatorSettings( 
    /* [in] */ __RPC__in_opt const IFabricCodePackageActivationContext *codePackageActivationContext,
    /* [in] */ __RPC__in LPCWSTR configurationPackageName,
    /* [in] */ __RPC__in LPCWSTR sectionName,
    /* [retval][out] */ __RPC__deref_out_opt IFabricReplicatorSettingsResult **replicatorSettings);

/* [entry] */ HRESULT FabricLoadSecurityCredentials( 
    /* [in] */ __RPC__in_opt const IFabricCodePackageActivationContext *codePackageActivationContext,
    /* [in] */ __RPC__in LPCWSTR configurationPackageName,
    /* [in] */ __RPC__in LPCWSTR sectionName,
    /* [retval][out] */ __RPC__deref_out_opt IFabricSecurityCredentialsResult **securityCredentials);

/* [entry] */ HRESULT FabricLoadEseLocalStoreSettings( 
    /* [in] */ __RPC__in_opt const IFabricCodePackageActivationContext *codePackageActivationContext,
    /* [in] */ __RPC__in LPCWSTR configurationPackageName,
    /* [in] */ __RPC__in LPCWSTR sectionName,
    /* [retval][out] */ __RPC__deref_out_opt IFabricEseLocalStoreSettingsResult **settings);

/* [entry] */ HRESULT FabricBeginGetCodePackageActivator( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ __RPC__in_opt IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ __RPC__deref_out_opt IFabricAsyncOperationContext **context);

/* [entry] */ HRESULT FabricEndGetCodePackageActivator( 
    /* [in] */ __RPC__in_opt IFabricAsyncOperationContext *context,
    /* [retval][out] */ __RPC__deref_out_opt void **activator);

/* [entry] */ HRESULT FabricGetCodePackageActivator( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **activator);

#endif /* __FabricRuntimeModule_MODULE_DEFINED__ */
#endif /* __FabricRuntimeLib_LIBRARY_DEFINED__ */

#ifndef __IFabricStateReplicator2_INTERFACE_DEFINED__
#define __IFabricStateReplicator2_INTERFACE_DEFINED__

/* interface IFabricStateReplicator2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStateReplicator2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4A28D542-658F-46F9-9BF4-79B7CAE25C5D")
    IFabricStateReplicator2 : public IFabricStateReplicator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetReplicatorSettings( 
            /* [retval][out] */ IFabricReplicatorSettingsResult **replicatorSettings) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStateReplicator2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStateReplicator2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStateReplicator2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStateReplicator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicate )( 
            IFabricStateReplicator2 * This,
            /* [in] */ IFabricOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReplicate )( 
            IFabricStateReplicator2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *GetReplicationStream )( 
            IFabricStateReplicator2 * This,
            /* [retval][out] */ IFabricOperationStream **stream);
        
        HRESULT ( STDMETHODCALLTYPE *GetCopyStream )( 
            IFabricStateReplicator2 * This,
            /* [retval][out] */ IFabricOperationStream **stream);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricStateReplicator2 * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *GetReplicatorSettings )( 
            IFabricStateReplicator2 * This,
            /* [retval][out] */ IFabricReplicatorSettingsResult **replicatorSettings);
        
        END_INTERFACE
    } IFabricStateReplicator2Vtbl;

    interface IFabricStateReplicator2
    {
        CONST_VTBL struct IFabricStateReplicator2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStateReplicator2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStateReplicator2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStateReplicator2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStateReplicator2_BeginReplicate(This,operationData,callback,sequenceNumber,context)	\
    ( (This)->lpVtbl -> BeginReplicate(This,operationData,callback,sequenceNumber,context) ) 

#define IFabricStateReplicator2_EndReplicate(This,context,sequenceNumber)	\
    ( (This)->lpVtbl -> EndReplicate(This,context,sequenceNumber) ) 

#define IFabricStateReplicator2_GetReplicationStream(This,stream)	\
    ( (This)->lpVtbl -> GetReplicationStream(This,stream) ) 

#define IFabricStateReplicator2_GetCopyStream(This,stream)	\
    ( (This)->lpVtbl -> GetCopyStream(This,stream) ) 

#define IFabricStateReplicator2_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 


#define IFabricStateReplicator2_GetReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> GetReplicatorSettings(This,replicatorSettings) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStateReplicator2_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageChangeHandler_INTERFACE_DEFINED__
#define __IFabricCodePackageChangeHandler_INTERFACE_DEFINED__

/* interface IFabricCodePackageChangeHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageChangeHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b90d36cd-acb5-427a-b318-3b045981d0cc")
    IFabricCodePackageChangeHandler : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnPackageAdded( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricCodePackage *codePackage) = 0;
        
        virtual void STDMETHODCALLTYPE OnPackageRemoved( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricCodePackage *codePackage) = 0;
        
        virtual void STDMETHODCALLTYPE OnPackageModified( 
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricCodePackage *previousCodePackage,
            /* [in] */ IFabricCodePackage *codePackage) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageChangeHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageChangeHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageChangeHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageChangeHandler * This);
        
        void ( STDMETHODCALLTYPE *OnPackageAdded )( 
            IFabricCodePackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricCodePackage *codePackage);
        
        void ( STDMETHODCALLTYPE *OnPackageRemoved )( 
            IFabricCodePackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricCodePackage *codePackage);
        
        void ( STDMETHODCALLTYPE *OnPackageModified )( 
            IFabricCodePackageChangeHandler * This,
            /* [in] */ IFabricCodePackageActivationContext *source,
            /* [in] */ IFabricCodePackage *previousCodePackage,
            /* [in] */ IFabricCodePackage *codePackage);
        
        END_INTERFACE
    } IFabricCodePackageChangeHandlerVtbl;

    interface IFabricCodePackageChangeHandler
    {
        CONST_VTBL struct IFabricCodePackageChangeHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageChangeHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageChangeHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageChangeHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageChangeHandler_OnPackageAdded(This,source,codePackage)	\
    ( (This)->lpVtbl -> OnPackageAdded(This,source,codePackage) ) 

#define IFabricCodePackageChangeHandler_OnPackageRemoved(This,source,codePackage)	\
    ( (This)->lpVtbl -> OnPackageRemoved(This,source,codePackage) ) 

#define IFabricCodePackageChangeHandler_OnPackageModified(This,source,previousCodePackage,codePackage)	\
    ( (This)->lpVtbl -> OnPackageModified(This,source,previousCodePackage,codePackage) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageChangeHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica4_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplica4_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplica4 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplica4;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ff16d2f1-41a9-4c64-804a-a20bf28c04f3")
    IFabricKeyValueStoreReplica4 : public IFabricKeyValueStoreReplica3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRestore( 
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRestore( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplica4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplica4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplica4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricKeyValueStoreReplica4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentEpoch )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Update )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *Contains )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadata )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction2 )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS *settings,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *BeginBackup )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
            /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndBackup )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestore )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRestore )( 
            IFabricKeyValueStoreReplica4 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplica4Vtbl;

    interface IFabricKeyValueStoreReplica4
    {
        CONST_VTBL struct IFabricKeyValueStoreReplica4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplica4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplica4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplica4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplica4_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricKeyValueStoreReplica4_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricKeyValueStoreReplica4_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricKeyValueStoreReplica4_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricKeyValueStoreReplica4_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricKeyValueStoreReplica4_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricKeyValueStoreReplica4_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 


#define IFabricKeyValueStoreReplica4_GetCurrentEpoch(This,currentEpoch)	\
    ( (This)->lpVtbl -> GetCurrentEpoch(This,currentEpoch) ) 

#define IFabricKeyValueStoreReplica4_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#define IFabricKeyValueStoreReplica4_CreateTransaction(This,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction(This,transaction) ) 

#define IFabricKeyValueStoreReplica4_Add(This,transaction,key,valueSizeInBytes,value)	\
    ( (This)->lpVtbl -> Add(This,transaction,key,valueSizeInBytes,value) ) 

#define IFabricKeyValueStoreReplica4_Remove(This,transaction,key,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Remove(This,transaction,key,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica4_Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica4_Get(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Get(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica4_GetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> GetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica4_Contains(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Contains(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica4_Enumerate(This,transaction,result)	\
    ( (This)->lpVtbl -> Enumerate(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica4_EnumerateByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,transaction,keyPrefix,result) ) 

#define IFabricKeyValueStoreReplica4_EnumerateMetadata(This,transaction,result)	\
    ( (This)->lpVtbl -> EnumerateMetadata(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica4_EnumerateMetadataByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,transaction,keyPrefix,result) ) 


#define IFabricKeyValueStoreReplica4_Backup(This,backupDirectory)	\
    ( (This)->lpVtbl -> Backup(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica4_Restore(This,backupDirectory)	\
    ( (This)->lpVtbl -> Restore(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica4_CreateTransaction2(This,settings,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction2(This,settings,transaction) ) 


#define IFabricKeyValueStoreReplica4_BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context)	\
    ( (This)->lpVtbl -> BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context) ) 

#define IFabricKeyValueStoreReplica4_EndBackup(This,context)	\
    ( (This)->lpVtbl -> EndBackup(This,context) ) 


#define IFabricKeyValueStoreReplica4_BeginRestore(This,backupDirectory,callback,context)	\
    ( (This)->lpVtbl -> BeginRestore(This,backupDirectory,callback,context) ) 

#define IFabricKeyValueStoreReplica4_EndRestore(This,context)	\
    ( (This)->lpVtbl -> EndRestore(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplica4_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica5_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplica5_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplica5 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplica5;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34f2da40-6227-448a-be72-c517b0d69432")
    IFabricKeyValueStoreReplica5 : public IFabricKeyValueStoreReplica4
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TryAdd( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [retval][out] */ BOOLEAN *added) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TryRemove( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TryUpdate( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TryGet( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TryGetMetadata( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateByKey2( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey2( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplica5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplica5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplica5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricKeyValueStoreReplica5 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentEpoch )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Update )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *Contains )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadata )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction2 )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS *settings,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *BeginBackup )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
            /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndBackup )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestore )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRestore )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *TryAdd )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [retval][out] */ BOOLEAN *added);
        
        HRESULT ( STDMETHODCALLTYPE *TryRemove )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists);
        
        HRESULT ( STDMETHODCALLTYPE *TryUpdate )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists);
        
        HRESULT ( STDMETHODCALLTYPE *TryGet )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *TryGetMetadata )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey2 )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey2 )( 
            IFabricKeyValueStoreReplica5 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplica5Vtbl;

    interface IFabricKeyValueStoreReplica5
    {
        CONST_VTBL struct IFabricKeyValueStoreReplica5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplica5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplica5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplica5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplica5_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricKeyValueStoreReplica5_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricKeyValueStoreReplica5_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricKeyValueStoreReplica5_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricKeyValueStoreReplica5_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricKeyValueStoreReplica5_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricKeyValueStoreReplica5_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 


#define IFabricKeyValueStoreReplica5_GetCurrentEpoch(This,currentEpoch)	\
    ( (This)->lpVtbl -> GetCurrentEpoch(This,currentEpoch) ) 

#define IFabricKeyValueStoreReplica5_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#define IFabricKeyValueStoreReplica5_CreateTransaction(This,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction(This,transaction) ) 

#define IFabricKeyValueStoreReplica5_Add(This,transaction,key,valueSizeInBytes,value)	\
    ( (This)->lpVtbl -> Add(This,transaction,key,valueSizeInBytes,value) ) 

#define IFabricKeyValueStoreReplica5_Remove(This,transaction,key,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Remove(This,transaction,key,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica5_Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica5_Get(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Get(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica5_GetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> GetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica5_Contains(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Contains(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica5_Enumerate(This,transaction,result)	\
    ( (This)->lpVtbl -> Enumerate(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica5_EnumerateByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,transaction,keyPrefix,result) ) 

#define IFabricKeyValueStoreReplica5_EnumerateMetadata(This,transaction,result)	\
    ( (This)->lpVtbl -> EnumerateMetadata(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica5_EnumerateMetadataByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,transaction,keyPrefix,result) ) 


#define IFabricKeyValueStoreReplica5_Backup(This,backupDirectory)	\
    ( (This)->lpVtbl -> Backup(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica5_Restore(This,backupDirectory)	\
    ( (This)->lpVtbl -> Restore(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica5_CreateTransaction2(This,settings,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction2(This,settings,transaction) ) 


#define IFabricKeyValueStoreReplica5_BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context)	\
    ( (This)->lpVtbl -> BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context) ) 

#define IFabricKeyValueStoreReplica5_EndBackup(This,context)	\
    ( (This)->lpVtbl -> EndBackup(This,context) ) 


#define IFabricKeyValueStoreReplica5_BeginRestore(This,backupDirectory,callback,context)	\
    ( (This)->lpVtbl -> BeginRestore(This,backupDirectory,callback,context) ) 

#define IFabricKeyValueStoreReplica5_EndRestore(This,context)	\
    ( (This)->lpVtbl -> EndRestore(This,context) ) 


#define IFabricKeyValueStoreReplica5_TryAdd(This,transaction,key,valueSizeInBytes,value,added)	\
    ( (This)->lpVtbl -> TryAdd(This,transaction,key,valueSizeInBytes,value,added) ) 

#define IFabricKeyValueStoreReplica5_TryRemove(This,transaction,key,checkSequenceNumber,exists)	\
    ( (This)->lpVtbl -> TryRemove(This,transaction,key,checkSequenceNumber,exists) ) 

#define IFabricKeyValueStoreReplica5_TryUpdate(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber,exists)	\
    ( (This)->lpVtbl -> TryUpdate(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber,exists) ) 

#define IFabricKeyValueStoreReplica5_TryGet(This,transaction,key,result)	\
    ( (This)->lpVtbl -> TryGet(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica5_TryGetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> TryGetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica5_EnumerateByKey2(This,transaction,keyPrefix,strictPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey2(This,transaction,keyPrefix,strictPrefix,result) ) 

#define IFabricKeyValueStoreReplica5_EnumerateMetadataByKey2(This,transaction,keyPrefix,strictPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey2(This,transaction,keyPrefix,strictPrefix,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplica5_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplica6_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplica6_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplica6 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplica6;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("56e77be1-e81f-4e42-8522-162c2d608184")
    IFabricKeyValueStoreReplica6 : public IFabricKeyValueStoreReplica5
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRestore2( 
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS *settings,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplica6Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplica6 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplica6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT ( STDMETHODCALLTYPE *BeginChangeRole )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndChangeRole )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricKeyValueStoreReplica6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentEpoch )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Update )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *Contains )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *Enumerate )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadata )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *Backup )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ LPCWSTR backupDirectory);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransaction2 )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS *settings,
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT ( STDMETHODCALLTYPE *BeginBackup )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
            /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndBackup )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestore )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRestore )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *TryAdd )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [retval][out] */ BOOLEAN *added);
        
        HRESULT ( STDMETHODCALLTYPE *TryRemove )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists);
        
        HRESULT ( STDMETHODCALLTYPE *TryUpdate )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists);
        
        HRESULT ( STDMETHODCALLTYPE *TryGet )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *TryGetMetadata )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey2 )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey2 )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestore2 )( 
            IFabricKeyValueStoreReplica6 * This,
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS *settings,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplica6Vtbl;

    interface IFabricKeyValueStoreReplica6
    {
        CONST_VTBL struct IFabricKeyValueStoreReplica6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplica6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplica6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplica6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplica6_BeginOpen(This,openMode,partition,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,openMode,partition,callback,context) ) 

#define IFabricKeyValueStoreReplica6_EndOpen(This,context,replicator)	\
    ( (This)->lpVtbl -> EndOpen(This,context,replicator) ) 

#define IFabricKeyValueStoreReplica6_BeginChangeRole(This,newRole,callback,context)	\
    ( (This)->lpVtbl -> BeginChangeRole(This,newRole,callback,context) ) 

#define IFabricKeyValueStoreReplica6_EndChangeRole(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndChangeRole(This,context,serviceAddress) ) 

#define IFabricKeyValueStoreReplica6_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricKeyValueStoreReplica6_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricKeyValueStoreReplica6_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 


#define IFabricKeyValueStoreReplica6_GetCurrentEpoch(This,currentEpoch)	\
    ( (This)->lpVtbl -> GetCurrentEpoch(This,currentEpoch) ) 

#define IFabricKeyValueStoreReplica6_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#define IFabricKeyValueStoreReplica6_CreateTransaction(This,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction(This,transaction) ) 

#define IFabricKeyValueStoreReplica6_Add(This,transaction,key,valueSizeInBytes,value)	\
    ( (This)->lpVtbl -> Add(This,transaction,key,valueSizeInBytes,value) ) 

#define IFabricKeyValueStoreReplica6_Remove(This,transaction,key,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Remove(This,transaction,key,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica6_Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber)	\
    ( (This)->lpVtbl -> Update(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber) ) 

#define IFabricKeyValueStoreReplica6_Get(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Get(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica6_GetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> GetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica6_Contains(This,transaction,key,result)	\
    ( (This)->lpVtbl -> Contains(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica6_Enumerate(This,transaction,result)	\
    ( (This)->lpVtbl -> Enumerate(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica6_EnumerateByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,transaction,keyPrefix,result) ) 

#define IFabricKeyValueStoreReplica6_EnumerateMetadata(This,transaction,result)	\
    ( (This)->lpVtbl -> EnumerateMetadata(This,transaction,result) ) 

#define IFabricKeyValueStoreReplica6_EnumerateMetadataByKey(This,transaction,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,transaction,keyPrefix,result) ) 


#define IFabricKeyValueStoreReplica6_Backup(This,backupDirectory)	\
    ( (This)->lpVtbl -> Backup(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica6_Restore(This,backupDirectory)	\
    ( (This)->lpVtbl -> Restore(This,backupDirectory) ) 

#define IFabricKeyValueStoreReplica6_CreateTransaction2(This,settings,transaction)	\
    ( (This)->lpVtbl -> CreateTransaction2(This,settings,transaction) ) 


#define IFabricKeyValueStoreReplica6_BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context)	\
    ( (This)->lpVtbl -> BeginBackup(This,backupDirectory,backupOption,postBackupHandler,callback,context) ) 

#define IFabricKeyValueStoreReplica6_EndBackup(This,context)	\
    ( (This)->lpVtbl -> EndBackup(This,context) ) 


#define IFabricKeyValueStoreReplica6_BeginRestore(This,backupDirectory,callback,context)	\
    ( (This)->lpVtbl -> BeginRestore(This,backupDirectory,callback,context) ) 

#define IFabricKeyValueStoreReplica6_EndRestore(This,context)	\
    ( (This)->lpVtbl -> EndRestore(This,context) ) 


#define IFabricKeyValueStoreReplica6_TryAdd(This,transaction,key,valueSizeInBytes,value,added)	\
    ( (This)->lpVtbl -> TryAdd(This,transaction,key,valueSizeInBytes,value,added) ) 

#define IFabricKeyValueStoreReplica6_TryRemove(This,transaction,key,checkSequenceNumber,exists)	\
    ( (This)->lpVtbl -> TryRemove(This,transaction,key,checkSequenceNumber,exists) ) 

#define IFabricKeyValueStoreReplica6_TryUpdate(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber,exists)	\
    ( (This)->lpVtbl -> TryUpdate(This,transaction,key,valueSizeInBytes,value,checkSequenceNumber,exists) ) 

#define IFabricKeyValueStoreReplica6_TryGet(This,transaction,key,result)	\
    ( (This)->lpVtbl -> TryGet(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica6_TryGetMetadata(This,transaction,key,result)	\
    ( (This)->lpVtbl -> TryGetMetadata(This,transaction,key,result) ) 

#define IFabricKeyValueStoreReplica6_EnumerateByKey2(This,transaction,keyPrefix,strictPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey2(This,transaction,keyPrefix,strictPrefix,result) ) 

#define IFabricKeyValueStoreReplica6_EnumerateMetadataByKey2(This,transaction,keyPrefix,strictPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey2(This,transaction,keyPrefix,strictPrefix,result) ) 


#define IFabricKeyValueStoreReplica6_BeginRestore2(This,backupDirectory,settings,callback,context)	\
    ( (This)->lpVtbl -> BeginRestore2(This,backupDirectory,settings,callback,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplica6_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreEnumerator_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreEnumerator_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreEnumerator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreEnumerator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6722b848-15bb-4528-bf54-c7bbe27b6f9a")
    IFabricKeyValueStoreEnumerator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumerateByKey( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreEnumeratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreEnumerator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreEnumerator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreEnumerator * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreEnumerator * This,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreEnumerator * This,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        END_INTERFACE
    } IFabricKeyValueStoreEnumeratorVtbl;

    interface IFabricKeyValueStoreEnumerator
    {
        CONST_VTBL struct IFabricKeyValueStoreEnumeratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreEnumerator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreEnumerator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreEnumerator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreEnumerator_EnumerateByKey(This,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,keyPrefix,result) ) 

#define IFabricKeyValueStoreEnumerator_EnumerateMetadataByKey(This,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,keyPrefix,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreEnumerator_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreEnumerator2_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreEnumerator2_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreEnumerator2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreEnumerator2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("63dfd264-4f2b-4be6-8234-1fa200165fe9")
    IFabricKeyValueStoreEnumerator2 : public IFabricKeyValueStoreEnumerator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumerateByKey2( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey2( 
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreEnumerator2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreEnumerator2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreEnumerator2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey )( 
            IFabricKeyValueStoreEnumerator2 * This,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey )( 
            IFabricKeyValueStoreEnumerator2 * This,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateByKey2 )( 
            IFabricKeyValueStoreEnumerator2 * This,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT ( STDMETHODCALLTYPE *EnumerateMetadataByKey2 )( 
            IFabricKeyValueStoreEnumerator2 * This,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        END_INTERFACE
    } IFabricKeyValueStoreEnumerator2Vtbl;

    interface IFabricKeyValueStoreEnumerator2
    {
        CONST_VTBL struct IFabricKeyValueStoreEnumerator2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreEnumerator2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreEnumerator2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreEnumerator2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreEnumerator2_EnumerateByKey(This,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey(This,keyPrefix,result) ) 

#define IFabricKeyValueStoreEnumerator2_EnumerateMetadataByKey(This,keyPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey(This,keyPrefix,result) ) 


#define IFabricKeyValueStoreEnumerator2_EnumerateByKey2(This,keyPrefix,strictPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateByKey2(This,keyPrefix,strictPrefix,result) ) 

#define IFabricKeyValueStoreEnumerator2_EnumerateMetadataByKey2(This,keyPrefix,strictPrefix,result)	\
    ( (This)->lpVtbl -> EnumerateMetadataByKey2(This,keyPrefix,strictPrefix,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreEnumerator2_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemEnumerator2_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreItemEnumerator2_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreItemEnumerator2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreItemEnumerator2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("da143bbc-81e1-48cd-afd7-b642bc5b9bfd")
    IFabricKeyValueStoreItemEnumerator2 : public IFabricKeyValueStoreItemEnumerator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TryMoveNext( 
            /* [retval][out] */ BOOLEAN *success) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreItemEnumerator2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreItemEnumerator2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreItemEnumerator2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreItemEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFabricKeyValueStoreItemEnumerator2 * This);
        
        IFabricKeyValueStoreItemResult *( STDMETHODCALLTYPE *get_Current )( 
            IFabricKeyValueStoreItemEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TryMoveNext )( 
            IFabricKeyValueStoreItemEnumerator2 * This,
            /* [retval][out] */ BOOLEAN *success);
        
        END_INTERFACE
    } IFabricKeyValueStoreItemEnumerator2Vtbl;

    interface IFabricKeyValueStoreItemEnumerator2
    {
        CONST_VTBL struct IFabricKeyValueStoreItemEnumerator2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreItemEnumerator2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreItemEnumerator2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreItemEnumerator2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreItemEnumerator2_MoveNext(This)	\
    ( (This)->lpVtbl -> MoveNext(This) ) 

#define IFabricKeyValueStoreItemEnumerator2_get_Current(This)	\
    ( (This)->lpVtbl -> get_Current(This) ) 


#define IFabricKeyValueStoreItemEnumerator2_TryMoveNext(This,success)	\
    ( (This)->lpVtbl -> TryMoveNext(This,success) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreItemEnumerator2_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreItemMetadataEnumerator2_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreItemMetadataEnumerator2_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreItemMetadataEnumerator2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreItemMetadataEnumerator2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8803d53e-dd73-40fc-a662-1bfe999419ea")
    IFabricKeyValueStoreItemMetadataEnumerator2 : public IFabricKeyValueStoreItemMetadataEnumerator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TryMoveNext( 
            /* [retval][out] */ BOOLEAN *success) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreItemMetadataEnumerator2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreItemMetadataEnumerator2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreItemMetadataEnumerator2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreItemMetadataEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFabricKeyValueStoreItemMetadataEnumerator2 * This);
        
        IFabricKeyValueStoreItemMetadataResult *( STDMETHODCALLTYPE *get_Current )( 
            IFabricKeyValueStoreItemMetadataEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TryMoveNext )( 
            IFabricKeyValueStoreItemMetadataEnumerator2 * This,
            /* [retval][out] */ BOOLEAN *success);
        
        END_INTERFACE
    } IFabricKeyValueStoreItemMetadataEnumerator2Vtbl;

    interface IFabricKeyValueStoreItemMetadataEnumerator2
    {
        CONST_VTBL struct IFabricKeyValueStoreItemMetadataEnumerator2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreItemMetadataEnumerator2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreItemMetadataEnumerator2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreItemMetadataEnumerator2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreItemMetadataEnumerator2_MoveNext(This)	\
    ( (This)->lpVtbl -> MoveNext(This) ) 

#define IFabricKeyValueStoreItemMetadataEnumerator2_get_Current(This)	\
    ( (This)->lpVtbl -> get_Current(This) ) 


#define IFabricKeyValueStoreItemMetadataEnumerator2_TryMoveNext(This,success)	\
    ( (This)->lpVtbl -> TryMoveNext(This,success) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreItemMetadataEnumerator2_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreNotificationEnumerator2_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreNotificationEnumerator2_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreNotificationEnumerator2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreNotificationEnumerator2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("55eec7c6-ae81-407a-b84c-22771d314ac7")
    IFabricKeyValueStoreNotificationEnumerator2 : public IFabricKeyValueStoreNotificationEnumerator
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE TryMoveNext( 
            /* [retval][out] */ BOOLEAN *success) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreNotificationEnumerator2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This);
        
        IFabricKeyValueStoreNotification *( STDMETHODCALLTYPE *get_Current )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This);
        
        void ( STDMETHODCALLTYPE *Reset )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *TryMoveNext )( 
            IFabricKeyValueStoreNotificationEnumerator2 * This,
            /* [retval][out] */ BOOLEAN *success);
        
        END_INTERFACE
    } IFabricKeyValueStoreNotificationEnumerator2Vtbl;

    interface IFabricKeyValueStoreNotificationEnumerator2
    {
        CONST_VTBL struct IFabricKeyValueStoreNotificationEnumerator2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreNotificationEnumerator2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreNotificationEnumerator2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreNotificationEnumerator2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreNotificationEnumerator2_MoveNext(This)	\
    ( (This)->lpVtbl -> MoveNext(This) ) 

#define IFabricKeyValueStoreNotificationEnumerator2_get_Current(This)	\
    ( (This)->lpVtbl -> get_Current(This) ) 

#define IFabricKeyValueStoreNotificationEnumerator2_Reset(This)	\
    ( (This)->lpVtbl -> Reset(This) ) 


#define IFabricKeyValueStoreNotificationEnumerator2_TryMoveNext(This,success)	\
    ( (This)->lpVtbl -> TryMoveNext(This,success) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreNotificationEnumerator2_INTERFACE_DEFINED__ */


#ifndef __IFabricStoreEventHandler_INTERFACE_DEFINED__
#define __IFabricStoreEventHandler_INTERFACE_DEFINED__

/* interface IFabricStoreEventHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStoreEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("220e6da4-985b-4dee-8fe9-77521b838795")
    IFabricStoreEventHandler : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE OnDataLoss( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStoreEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStoreEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStoreEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStoreEventHandler * This);
        
        void ( STDMETHODCALLTYPE *OnDataLoss )( 
            IFabricStoreEventHandler * This);
        
        END_INTERFACE
    } IFabricStoreEventHandlerVtbl;

    interface IFabricStoreEventHandler
    {
        CONST_VTBL struct IFabricStoreEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStoreEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStoreEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStoreEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStoreEventHandler_OnDataLoss(This)	\
    ( (This)->lpVtbl -> OnDataLoss(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStoreEventHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricStoreEventHandler2_INTERFACE_DEFINED__
#define __IFabricStoreEventHandler2_INTERFACE_DEFINED__

/* interface IFabricStoreEventHandler2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStoreEventHandler2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CCE4523F-614B-4D6A-98A3-1E197C0213EA")
    IFabricStoreEventHandler2 : public IFabricStoreEventHandler
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStoreEventHandler2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStoreEventHandler2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStoreEventHandler2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStoreEventHandler2 * This);
        
        void ( STDMETHODCALLTYPE *OnDataLoss )( 
            IFabricStoreEventHandler2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOnDataLoss )( 
            IFabricStoreEventHandler2 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOnDataLoss )( 
            IFabricStoreEventHandler2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged);
        
        END_INTERFACE
    } IFabricStoreEventHandler2Vtbl;

    interface IFabricStoreEventHandler2
    {
        CONST_VTBL struct IFabricStoreEventHandler2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStoreEventHandler2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStoreEventHandler2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStoreEventHandler2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStoreEventHandler2_OnDataLoss(This)	\
    ( (This)->lpVtbl -> OnDataLoss(This) ) 


#define IFabricStoreEventHandler2_BeginOnDataLoss(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOnDataLoss(This,callback,context) ) 

#define IFabricStoreEventHandler2_EndOnDataLoss(This,context,isStateChanged)	\
    ( (This)->lpVtbl -> EndOnDataLoss(This,context,isStateChanged) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStoreEventHandler2_INTERFACE_DEFINED__ */


#ifndef __IFabricStorePostBackupHandler_INTERFACE_DEFINED__
#define __IFabricStorePostBackupHandler_INTERFACE_DEFINED__

/* interface IFabricStorePostBackupHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStorePostBackupHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2af2e8a6-41df-4e32-9d2a-d73a711e652a")
    IFabricStorePostBackupHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginPostBackup( 
            /* [in] */ FABRIC_STORE_BACKUP_INFO *info,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPostBackup( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *status) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStorePostBackupHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStorePostBackupHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStorePostBackupHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStorePostBackupHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPostBackup )( 
            IFabricStorePostBackupHandler * This,
            /* [in] */ FABRIC_STORE_BACKUP_INFO *info,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndPostBackup )( 
            IFabricStorePostBackupHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *status);
        
        END_INTERFACE
    } IFabricStorePostBackupHandlerVtbl;

    interface IFabricStorePostBackupHandler
    {
        CONST_VTBL struct IFabricStorePostBackupHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStorePostBackupHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStorePostBackupHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStorePostBackupHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStorePostBackupHandler_BeginPostBackup(This,info,callback,context)	\
    ( (This)->lpVtbl -> BeginPostBackup(This,info,callback,context) ) 

#define IFabricStorePostBackupHandler_EndPostBackup(This,context,status)	\
    ( (This)->lpVtbl -> EndPostBackup(This,context,status) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStorePostBackupHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricSecondaryEventHandler_INTERFACE_DEFINED__
#define __IFabricSecondaryEventHandler_INTERFACE_DEFINED__

/* interface IFabricSecondaryEventHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricSecondaryEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7d124a7d-258e-49f2-a9b0-e800406103fb")
    IFabricSecondaryEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnCopyComplete( 
            /* [in] */ IFabricKeyValueStoreEnumerator *enumerator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnReplicationOperation( 
            /* [in] */ IFabricKeyValueStoreNotificationEnumerator *enumerator) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricSecondaryEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricSecondaryEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricSecondaryEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricSecondaryEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnCopyComplete )( 
            IFabricSecondaryEventHandler * This,
            /* [in] */ IFabricKeyValueStoreEnumerator *enumerator);
        
        HRESULT ( STDMETHODCALLTYPE *OnReplicationOperation )( 
            IFabricSecondaryEventHandler * This,
            /* [in] */ IFabricKeyValueStoreNotificationEnumerator *enumerator);
        
        END_INTERFACE
    } IFabricSecondaryEventHandlerVtbl;

    interface IFabricSecondaryEventHandler
    {
        CONST_VTBL struct IFabricSecondaryEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricSecondaryEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricSecondaryEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricSecondaryEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricSecondaryEventHandler_OnCopyComplete(This,enumerator)	\
    ( (This)->lpVtbl -> OnCopyComplete(This,enumerator) ) 

#define IFabricSecondaryEventHandler_OnReplicationOperation(This,enumerator)	\
    ( (This)->lpVtbl -> OnReplicationOperation(This,enumerator) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricSecondaryEventHandler_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_fabricruntime_0000_0075 */
/* [local] */ 

typedef HRESULT (*FnFabricMain)(IFabricRuntime * runtime, IFabricCodePackageActivationContext * activationContext);


extern RPC_IF_HANDLE __MIDL_itf_fabricruntime_0000_0075_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricruntime_0000_0075_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


