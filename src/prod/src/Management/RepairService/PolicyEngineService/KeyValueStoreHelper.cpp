// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;
using namespace Common;

const std::wstring KeyValueStoreHelper::ReplicatorEndpointName = L"RepairPolicyEngineService_ReplicatorEndpoint";

bool KeyValueStoreHelper::GetReplicatorAddress(Common::ComPointer<IFabricCodePackageActivationContext> activationContextCPtr, 
                                               _Out_ std::wstring& replicatorAddress)
{
    replicatorAddress.clear();
    const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION *endpointResource = NULL;
    activationContextCPtr->GetServiceEndpointResource(ReplicatorEndpointName.c_str(), &endpointResource);
    if (endpointResource == NULL)
    {
        Trace.WriteError(ComponentName, "KeyValueStoreHelper::GetReplicatorAddress(). Unable to get the endpoint resource for endpoint name = {0}", ReplicatorEndpointName);
        return false;
    }

    // Find the IP address of current node
    ComPointer<IFabricNodeContextResult2> nodeContextCPtr;
    HRESULT hr = ::FabricGetNodeContext(nodeContextCPtr.VoidInitializationAddress());
    if (FAILED(hr))
    {
        Trace.WriteError(ComponentName, "KeyValueStoreHelper::GetReplicatorAddress() failed to get node context (error = {0})", Common::ErrorCode::FromHResult(hr));
        return false;
    }

    replicatorAddress = Transport::TcpTransportUtility::ConstructAddressString(
        nodeContextCPtr->get_NodeContext()->IPAddressOrFQDN,
        endpointResource->Port);

    Trace.WriteInfo(ComponentName, "KeyValueStoreHelper::GetReplicatorAddress(). Replicator Address = {0}", replicatorAddress);

    return true;
}

bool KeyValueStoreHelper::LoadFromPersistentStore(NodeRepairInfo& nodeRepairInfo, 
                                                  Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr)
{
    CODING_ERROR_ASSERT(keyValueStorePtr != NULL && keyValueStorePtr.GetRawPointer() != nullptr);
    
    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore()");

    Common::ComPointer<IFabricTransaction> transaction;
    HRESULT hr = keyValueStorePtr->CreateTransaction(transaction.InitializationAddress());
    if (FAILED(hr))
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() Error in CreateTransaction(): {0}", Common::ErrorCode::FromHResult(hr));
        return false;
    }

    BOOLEAN isExistedInStore = 0;
    hr = keyValueStorePtr->Contains(transaction.GetRawPointer(), nodeRepairInfo.NodeName.c_str(), &isExistedInStore);
    if (FAILED(hr)) 
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() Error in Contains(): {0}", Common::ErrorCode::FromHResult(hr));
        return false;
    }
    if (!isExistedInStore)
    {
        Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() no data in key value store. Returning");
        return true;
    }

    Common::ComPointer<IFabricKeyValueStoreItemResult> result;
    hr = keyValueStorePtr->Get(transaction.GetRawPointer(), nodeRepairInfo.NodeName.c_str(), result.InitializationAddress());
    if (FAILED(hr))
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() Error in Get(): {0}", Common::ErrorCode::FromHResult(hr));
        return false;
    }

    const FABRIC_KEY_VALUE_STORE_ITEM *item = result->get_Item();
    if (item == NULL || item->Value == NULL || item->Metadata->ValueSizeInBytes == 0)
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() Invalid item from KVS Item Result.");
        return false;
    }
    ErrorCode errorCode = FabricSerializer::Deserialize(nodeRepairInfo,item->Metadata->ValueSizeInBytes,item->Value);
    if (!errorCode.IsSuccess())
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() Rolling back... Error in Deserialize() : {0}",errorCode);
        return false;
        
    }
    
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::LoadFromPersistentStore() Deserialization is successful for node {0}", nodeRepairInfo.NodeName);
    nodeRepairInfo.TraceState();
    return true;
}

HRESULT KeyValueStoreHelper::WriteToPersistentStore(NodeRepairInfo& nodeRepairInfo, 
                                                 Common::ComPointer<IFabricKeyValueStoreReplica2>  keyValueStorePtr, 
                                                 const RepairPolicyEngineConfiguration& repairConfiguration)
{
    int numberOfTimes = 0;
    HRESULT hr = S_OK;
    const int sleepInterval = 50;
    const int numberoftries = 5;
    // retry few times if it fails
    do
    {
        hr = WriteToPersistentStoreInternal(nodeRepairInfo,keyValueStorePtr,repairConfiguration);
        numberOfTimes++;
        
        // Add a sleep back off if failed before retrying
        if((FAILED(hr)) && numberOfTimes < numberoftries && hr != FABRIC_E_NOT_PRIMARY)
        {
            ::Sleep(sleepInterval * numberOfTimes);
        }
    }while(FAILED(hr) && numberOfTimes < numberoftries && hr != FABRIC_E_NOT_PRIMARY);
    
    // Retries failed
    if(FAILED(hr) && hr != FABRIC_E_NOT_PRIMARY)
    {
        Trace.WriteError(ComponentName, "RepairPolicyEngineServiceReplica::WriteToPersistentStore(). Failed after retries. Error = {0}", Common::ErrorCode::FromHResult(hr));
        keyValueStorePtr->Abort();
        ::ExitProcess(EXIT_FAILURE);
    }
    return hr;
}


HRESULT KeyValueStoreHelper::WriteToPersistentStoreInternal(NodeRepairInfo& nodeRepairInfo, 
                                                 Common::ComPointer<IFabricKeyValueStoreReplica2>  keyValueStorePtr, 
                                                 const RepairPolicyEngineConfiguration& repairConfiguration)
{
    CODING_ERROR_ASSERT(keyValueStorePtr != NULL && keyValueStorePtr.GetRawPointer() != nullptr);

    Trace.WriteNoise(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore()");

    Common::ComPointer<IFabricTransaction> transaction;
    HRESULT hr = keyValueStorePtr->CreateTransaction(transaction.InitializationAddress());
    if (FAILED(hr))
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Error in CreateTransaction(): {0}", Common::ErrorCode::FromHResult(hr));
        return hr;
    }
    
    BOOLEAN isExistedInStore = 0;
    hr = keyValueStorePtr->Contains(transaction.GetRawPointer(), nodeRepairInfo.NodeName.c_str(), &isExistedInStore);
    if (FAILED(hr)) 
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Error in Contains(): {0}", Common::ErrorCode::FromHResult(hr));
        return hr;
    }

    std::vector<byte> rawData;
    rawData.clear();
    Common::ErrorCode errorCode = FabricSerializer::Serialize(&nodeRepairInfo,rawData);
    if (!errorCode.IsSuccess())
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Serialization failed with error: {0}",errorCode);
        return errorCode.ToHResult();
        
    }
    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Serialization is successful, size = {0}", rawData.size());

    if (isExistedInStore)
    {
        Common::ComPointer<IFabricKeyValueStoreItemResult> result;
        hr = keyValueStorePtr->Get(transaction.GetRawPointer(), nodeRepairInfo.NodeName.c_str(), result.InitializationAddress());
        if (FAILED(hr))
        {
            Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Error in Get(): {0}", Common::ErrorCode::FromHResult(hr));
            return hr;
        }
        
        hr = keyValueStorePtr->Update(transaction.GetRawPointer(), 
                                      nodeRepairInfo.NodeName.c_str(), 
                                      static_cast<LONG>(rawData.size()),
                                      rawData.data(), 
                                      result->get_Item()->Metadata->SequenceNumber);
        if (FAILED(hr)) 
        {
            Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Rolling back... Error in Update(): {0}", Common::ErrorCode::FromHResult(hr));
            return hr;
        }
    }
    else
    {
        hr = keyValueStorePtr->Add(transaction.GetRawPointer(), nodeRepairInfo.NodeName.c_str(), static_cast<LONG>(rawData.size()),rawData.data());
        if (FAILED(hr)) 
        {
            Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Rolling back... Error in Add(): {0}", Common::ErrorCode::FromHResult(hr));
            return hr;
        }
    }
    
    ComPointer<CommitTransactionOperationCallback> callback = make_com<CommitTransactionOperationCallback>();
    callback->Initialize(transaction);

    ComPointer<IFabricAsyncOperationContext> commitContext;
    DWORD timeout = static_cast<int>(repairConfiguration.WindowsFabricQueryTimeoutInterval.TotalPositiveMilliseconds());
    hr = transaction->BeginCommit(timeout, callback.GetRawPointer(), commitContext.InitializationAddress());
    if (FAILED(hr))
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Error in BeginCommit(): {0}", Common::ErrorCode::FromHResult(hr)); 
        return hr;
    }
    hr = callback->Wait();
    if (FAILED(hr))
    {
        Trace.WriteWarning(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore() Rolling back... Error in callback Wait(): {0}", Common::ErrorCode::FromHResult(hr)); 
        transaction->Rollback();
        return hr;
    }

    Trace.WriteInfo(ComponentName, "RepairPolicyEngineService::WriteToPersistentStore(). Successful. Node: {0}. Commited sequence number: {1}", nodeRepairInfo.NodeName, callback->Result);
    return S_OK;
}
