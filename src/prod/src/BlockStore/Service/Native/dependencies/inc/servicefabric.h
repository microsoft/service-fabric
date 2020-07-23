// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <cstdint>

//============================================================================
// Service Fabric Type Definitions
//============================================================================

#ifdef _WIN32

#define RELIABLE_DICTIONARY_INTEROP_DLL "ReliableCollectionRuntime.dll"

#if defined(RELIABLE_COLLECTION_TEST)
#define RELIABLE_SERVICES_DLL "ReliableCollectionServiceStandalone.dll"
#else // !RELIABLE_COLLECTION_TEST
#define RELIABLE_SERVICES_DLL "ReliableCollectionService.dll"
#endif // RELIABLE_COLLECTION_TEST

#define TEST_HELPER_DLL "ReliableCollectionServiceStandalone.dll"

#else 

#define RELIABLE_DICTIONARY_INTEROP_DLL "libReliableCollectionRuntime.so"

#if defined(RELIABLE_COLLECTION_TEST)
#define RELIABLE_SERVICES_DLL "libReliableCollectionServiceStandalone.so"
#else // !defined(RELIABLE_COLLECTION_TEST)
#define RELIABLE_SERVICES_DLL "libReliableCollectionService.so"
#endif // defined(RELIABLE_COLLECTION_TEST)

#define TEST_HELPER_DLL "libReliableCollectionServiceStandalone.so"

#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "external/ReliableCollectionRuntime.h"
#include "external/ReliableCollectionRuntime.Internal.h"

extern "C" {
    typedef void *PartitionHandle;

    #ifndef _WIN32 
        #define S_OK 0
        #define SUCCEEDED(hr) ((hr) >= 0)
        #define FAILED(hr) ((hr) < 0)
    #endif

    typedef int64_t KEY_TYPE;

    //
    // Reliable Service definitions
    //
    typedef void(*RemovePartitionContextCallback)(KEY_TYPE key);
    typedef void(*AddPartitionContextCallback)(KEY_TYPE key, TxnReplicatorHandle txReplicator, PartitionHandle partition, GUID partitionId, int64_t replicaId);
    typedef void(*ChangeRoleCallback)(KEY_TYPE key, int32_t newRole);
    typedef void(*AbortPartitionContextCallback)(KEY_TYPE key);

    HRESULT ReliableCollectionService_InitializeEx(LPCU16STR serviceTypeName, 
                                                   int port, 
                                                   AddPartitionContextCallback add, 
                                                   RemovePartitionContextCallback remove,
                                                   ChangeRoleCallback change, 
                                                   AbortPartitionContextCallback abort,
                                                   BOOL registerManifestEndpoints,
                                                   BOOL skipUserPortEndpointRegistration,
                                                   BOOL reportEndpointsOnlyOnPrimaryReplica);

    typedef HRESULT (*pfnReliableCollectionService_InitializeEx)(LPCU16STR serviceTypeName,
                                                                 int port, 
                                                                 AddPartitionContextCallback add,
                                                                 RemovePartitionContextCallback remove, 
                                                                 ChangeRoleCallback change,
                                                                 AbortPartitionContextCallback abort,
                                                                 BOOL registerManifestEndpoints,
                                                                 BOOL skipUserPortEndpointRegistration,
                                                                 BOOL reportEndpointsOnlyOnPrimaryReplica);

    typedef HRESULT (*pfnReliableCollectionRuntime_Initialize) (uint16_t apiVersion);

    typedef HRESULT(*pfnReliableCollectionTxnReplicator_GetInfo)(TxnReplicatorHandle txnReplicator, TxnReplicator_Info* info);

    typedef HRESULT(*pfnReliableCollectionService_Cleanup)();
    
    //
    // Test helper
    //
    typedef HRESULT (*pfnInitializeTest)(
        LPCU16STR wszName, 
        BOOL allowRecovery,
        TxnReplicatorHandle *pReplicator, 
        PartitionHandle  *pPartition,
        GUID *partitionId,
        int64_t *replicaId
    );

    typedef void (*pfnCleanupTest)(
        LPCU16STR wszName, 
        TxnReplicatorHandle *pReplicator, 
        PartitionHandle  *pPartition
    );
}


