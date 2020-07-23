// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Management;
using namespace FileStoreService;

StringLiteral const TraceComponent("FileStoreServiceReplica");

FileStoreServiceReplica::FileStoreServiceReplica(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const & serviceName,
    __in ServiceRoutingAgentProxy & routingAgentProxy,
    Api::IClientFactoryPtr const & clientFactory,
    NodeInfo const & nodeInfo,
    FileStoreServiceFactoryHolder const & factoryHolder)
    : KeyValueStoreReplica(partitionId, replicaId)
    , serviceName_(SystemServiceApplicationNameHelper::GetInternalServiceName(serviceName))
    , storeRoot_(Path::Combine(Path::Combine(nodeInfo.WorkingDirectory, Constants::StoreRootDirectoryName), wformatString("{0}", replicaId)))
    , stagingRoot_(Path::Combine(Path::Combine(nodeInfo.WorkingDirectory, Constants::StagingRootDirectoryName), wformatString("{0}", replicaId)))
    , storeShareRoot_(Utility::GetSharePath(nodeInfo.IPAddressOrFQDN, wformatString("{0}_{1}", Constants::StoreShareName, nodeInfo.NodeName), replicaId))
    , stagingShareRoot_(Utility::GetSharePath(nodeInfo.IPAddressOrFQDN, wformatString("{0}_{1}", Constants::StagingShareName, nodeInfo.NodeName), replicaId))
#if defined(PLATFORM_UNIX)
    , ipAddressOrFQDN(nodeInfo.IPAddressOrFQDN)
#endif
    , requestManager_()
    , replicationManager_()    
    , serializedInfo_()
    , serviceLocation_()
    , clientFactory_(clientFactory)
    , fileStoreClient_()
    , propertyManagmentClient_()
    , lock_()
    , nodeInstance_(nodeInfo.NodeInstance)
    , factoryHolder_(factoryHolder)
    , routingAgentProxy_(routingAgentProxy)
    , fileStoreServiceCounters_()
{
    this->SetTraceId(this->PartitionedReplicaId.TraceId);

    WriteInfo(
        TraceComponent,
        TraceId,
        "ctor: node = {0} this = {1}",
        this->NodeInstance,
        static_cast<void*>(this));
}

FileStoreServiceReplica::~FileStoreServiceReplica()
{
    WriteInfo(
        TraceComponent,
        this->TraceId,
        "~dtor: node = {0} this = {1}",
        this->NodeInstance,
        static_cast<void*>(this));
}

// *******************
// StatefulServiceBase
// *******************

ErrorCode FileStoreServiceReplica::OnOpen(ComPointer<IFabricStatefulServicePartition> const & servicePartition)
{
    UNREFERENCED_PARAMETER(servicePartition);    

    auto error = clientFactory_->CreateInternalFileStoreServiceClient(fileStoreClient_);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "Failed to create FileStoreServiceClient. Error:{0}",
            error);
        return error;
    }

    error = clientFactory_->CreatePropertyManagementClient(propertyManagmentClient_);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "Failed to create FileStoreServiceClient. Error:{0}",
            error);
        return error;
    }

    this->fileStoreServiceCounters_ = FileStoreServiceCounters::CreateInstance(
        this->PartitionedReplicaId.TraceId + L":" + StringUtility::ToWString(DateTime::Now().Ticks));

    error = Utility::RetriableOperation([this]() { return this->CreateName(); }, 3);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "Failed to CreateName during open of replica. Error:{0}",
            error);
        return error;
    }    

    // Cache service location since it will not change after open
    serviceLocation_ = SystemServiceLocation(
        this->NodeInstance,
        this->PartitionId,
        this->ReplicaId,
        DateTime::Now().Ticks);

    // Serialized FileStorePartitionInfo
    // A copy of this data is used to regiter the service location
    // to naming whenever this replica ChangeRoles to primary
    FileStorePartitionInfo info(serviceLocation_.Location);
    vector<BYTE> serializedData;
    error = FabricSerializer::Serialize(&info, serializedData);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "Failed to serialize FileStorePartitionInfo: {0}. Error:{1}",
            info,
            error);
        return error;
    }

    serializedInfo_ = move(serializedData);

    error = ImpersonatedSMBCopyContext::Create(smbCopyContext_);
    if(!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "Failed to create ImpersonatedSMBCopyContext during open of replica. Error:{0}",
            error);
        return error;
    }

    ReplicationManagerSPtr replicationManager = make_shared<ReplicationManager>(
        ImageStoreServiceReplicaHolder(*this, this->CreateComponentRoot()));
    
    {
        AcquireWriteLock lock(lock_);
        replicationManager_ = move(replicationManager);
    }

#if !defined(PLATFORM_UNIX)
    {
        bool enableLoggingOfDiskSpace = FileStoreServiceConfig::GetConfig().EnableLoggingOfDiskSpace;
        WriteInfo(
            TraceComponent,
            TraceId,
            "FileStoreService : enable logging of free disk space - enableLoggingOfDiskSpace: {0}", enableLoggingOfDiskSpace);

        if (enableLoggingOfDiskSpace)
        {
            AcquireWriteLock lock(freeSpaceLoggingtimerLock_);
            freeSpaceLoggingTimer_ = Timer::Create(
                TimerTagDefault,
                [this](TimerSPtr const &)
            {
                LogFreeSpaceForDisks();
            },
                false);

            this->freeSpaceLoggingTimer_->Change(FileStoreServiceConfig::GetConfig().FreeDiskSpaceLoggingInterval, FileStoreServiceConfig::GetConfig().FreeDiskSpaceLoggingInterval);
        }
    }
#endif

    return ErrorCodeValue::Success;
}

ErrorCode FileStoreServiceReplica::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out wstring & serviceLocation)
{
    WriteInfo(
        TraceComponent,
        this->TraceId,
        "{0} OnChangeRole started. NodeInstance =  {1}, NewRole = {2}",
        static_cast<void*>(this),
        this->NodeInstance,
        static_cast<int>(newRole)); 

    ErrorCode error(ErrorCodeValue::Success);
    if (newRole == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {   
        int64 sequenceNumber;
        error = Utility::RetriableOperation([this, &sequenceNumber]() { return this->GetServiceLocationSequenceNumber(sequenceNumber); }, 3);  
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "Failed to get ServiceLocation Property metadata with Naming. Error:{0}",
                error);            
            return error;
        }

        error = Utility::RetriableOperation([this, sequenceNumber]() { return this->RegisterServiceLocation(sequenceNumber); }, 3);        
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "Failed to register ServiceLocation with Naming. Error:{0}",
                error);            
            return error;
        }

        FABRIC_EPOCH currentEpoch = {0};
        error = this->GetCurrentEpoch(currentEpoch);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "Failed to get current epoch. Error:{0}",
                error);            
            return error;
        }

        int64 epochDataLossNumber = currentEpoch.DataLossNumber;
        int64 epochConfigurationNumber = currentEpoch.ConfigurationNumber;
        {
            AcquireWriteLock lock(lock_);
            ASSERT_IF(requestManager_, "RequestManager should not exist");
#if !defined(PLATFORM_UNIX)
            requestManager_ = make_shared<RequestManager>(
                ImageStoreServiceReplicaHolder(*this, this->CreateComponentRoot()),
                this->stagingShareRoot_,
                this->storeShareRoot_,
                epochDataLossNumber,
                epochConfigurationNumber,
                routingAgentProxy_);
#else
            wstring stagingShareRoot = wformatString("{0}:{1}", ipAddressOrFQDN, this->stagingRoot_);
            wstring storeShareRoot =  wformatString("{0}:{1}", ipAddressOrFQDN, this->storeRoot_);
            std::replace(stagingShareRoot.begin(), stagingShareRoot.end(), '\\', '/');
            std::replace(storeShareRoot.begin(), storeShareRoot.end(), '\\', '/');
            requestManager_ = make_shared<RequestManager>(
                    ImageStoreServiceReplicaHolder(*this, this->CreateComponentRoot()),
                    stagingShareRoot,
                    storeShareRoot,
                    epochDataLossNumber,
                    epochConfigurationNumber,
                    routingAgentProxy_);
#endif
        }

        error = requestManager_->Open();
        if(!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "Failed to Open RequestManager. Error:{0}",
                error);
            return error; 
        }

        RegisterMessageHandler();

        serviceLocation = this->serviceLocation_.Location;
    }
    else
    {        
        RequestManagerSPtr requestManager;
        {
            AcquireWriteLock lock(lock_);
            if(requestManager_) { requestManager = move(requestManager_); }
        }

        if(requestManager)
        {
            UnregisterMessageHandler();

            error = requestManager->Close();
            if(!error.IsSuccess())
            {
                requestManager->AbortAndWaitForTermination();
            }
        }
    }

    // Endpoint must be returned during change role to Idle since ProcessCopyNotification() will block change role to Active
    //
    if (newRole == ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY || newRole == ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
#if !defined(PLATFORM_UNIX)
        serviceLocation = this->storeShareRoot_;
#else
        wstring storeShareRoot =  wformatString("{0}:{1}", ipAddressOrFQDN, this->storeRoot_);
        std::replace(storeShareRoot.begin(), storeShareRoot.end(), '\\', '/');
        serviceLocation = storeShareRoot;
#endif
    }

    if (newRole == ::FABRIC_REPLICA_ROLE_NONE)
    {
        DeleteStagingAndStoreFolder();   
    }
    
    WriteInfo(
        TraceComponent,
        this->TraceId,
        "{0} OnChangeRole completed. NodeInstance = {1}",
        static_cast<void*>(this),
        this->NodeInstance);

    return ErrorCodeValue::Success;
}

ErrorCode FileStoreServiceReplica::OnClose()
{
    ReplicationManagerSPtr replicationManager;
    {
        AcquireWriteLock lock(this->lock_);
       replicationManager = move(replicationManager_);
    }

    auto callback = this->OnCloseReplicaCallback;
    if (callback)
    {
        callback(this->PartitionId);
    }

#if !defined(PLATFORM_UNIX)
    CleanupFreeSpaceLoggingTimer();
#endif

    return ErrorCodeValue::Success;
}

void FileStoreServiceReplica::OnAbort()
{
    OnClose();
}

//
// IStoreEventHandler
//

AsyncOperationSPtr FileStoreServiceReplica::BeginOnDataLoss(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    RequestManagerSPtr requestManager;
    {
        AcquireReadLock lock(lock_);

        requestManager = requestManager_;
    }

    if (requestManager.get() != nullptr)
    {
        return requestManager->BeginOnDataLoss(callback, parent);
    }
    else
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }
}

ErrorCode FileStoreServiceReplica::EndOnDataLoss(
    __in AsyncOperationSPtr const & operation,
    __out bool & isStateChanged)
{
    RequestManagerSPtr requestManager;
    {
        AcquireReadLock lock(lock_);

        requestManager = requestManager_;
    }

    if (requestManager.get() != nullptr)
    {
        return requestManager->EndOnDataLoss(operation, isStateChanged);
    }
    else
    {
        isStateChanged = false;

        return CompletedAsyncOperation::End(operation);
    }
}

// **********************
// ISecondaryEventHandler
// **********************

ErrorCode FileStoreServiceReplica::OnCopyComplete(Api::IStoreEnumeratorPtr const & storeEnumerator)
{
    IStoreItemEnumeratorPtr storeItemEnumerator;
    auto error = storeEnumerator->Enumerate(Constants::StoreType::FileMetadata, L"", false, storeItemEnumerator);
    if(!error.IsSuccess())
    {
        if(error.IsError(ErrorCodeValue::NotFound))
        {
            // If store item enumerator is not found, no processing required
            return ErrorCodeValue::Success;
        }

        WriteWarning(
            TraceComponent,
            TraceId,
            "Reporting fault during copy since storeEnumerator retured error. Error:{0}",
            error);
        return ErrorCodeValue::FileStoreServiceReplicationProcessingError;
    }

    ReplicationManagerSPtr replicationManager;
    {
        AcquireReadLock lock(this->lock_);
        replicationManager = replicationManager_;
    }

    if(!replicationManager)
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "OnCopyComplete: ReplicationManager is not set.");
        return ErrorCodeValue::NotReady;
    }

    if(!replicationManager->ProcessCopyNotification(storeItemEnumerator))
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "Reporting fault during copy since ProcessCopyNotification failed.");
        return ErrorCodeValue::FileStoreServiceReplicationProcessingError;
    }

    return ErrorCodeValue::Success;
}

ErrorCode FileStoreServiceReplica::OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const & storeNotificationEnumerator)
{
    ReplicationManagerSPtr replicationManager;
    {
        AcquireReadLock lock(this->lock_);
        replicationManager = replicationManager_;
    }

    if(!replicationManager)
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "OnReplicationOperation: ReplicationManager is not set.");
        return ErrorCodeValue::NotReady;
    }

    if(!replicationManager->ProcessReplicationNotification(storeNotificationEnumerator))
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "Reporting fault during replication operation since ProcessReplicationNotification failed.");
        return ErrorCodeValue::FileStoreServiceReplicationProcessingError;
    }   

    return ErrorCodeValue::Success;
}

void FileStoreServiceReplica::RegisterMessageHandler()
{
    auto selfRoot = this->CreateComponentRoot();
    routingAgentProxy_.RegisterMessageHandler(
        serviceLocation_,
        [this, selfRoot](MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        {
            this->RequestMessageHandler(move(message), move(receiverContext));
        });        
}

void FileStoreServiceReplica::UnregisterMessageHandler()
{
    routingAgentProxy_.UnregisterMessageHandler(serviceLocation_);
}

void FileStoreServiceReplica::RequestMessageHandler(
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext)
{
    RequestManagerSPtr requestManager;
    {
        AcquireReadLock lock(lock_);
        if(requestManager_)
        {
            requestManager = requestManager_;
        }
    }

    if(requestManager)
    {
        requestManager->ProcessRequest(
            move(request),
            move(receiverContext));
    }
    else
    {
        FabricActivityHeader header;
        bool success = request->Headers.TryReadFirst(header);

        if(success)
        {
            auto activityId = FabricActivityHeader::FromMessage(*request).ActivityId;
            routingAgentProxy_.OnIpcFailure(ErrorCodeValue::FileStoreServiceNotReady, *receiverContext, activityId);
        }
        else
        {
            routingAgentProxy_.OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext);            
        }
    }
}

ErrorCode FileStoreServiceReplica::GetServiceLocationSequenceNumber(__out int64 & sequenceNumber)
{
    ErrorCode error;
    ManualResetEvent operationDone;

    propertyManagmentClient_->BeginGetPropertyMetadata(
        NamingUri(serviceName_),
        this->PartitionId.ToString(),
        FileStoreServiceConfig::GetConfig().NamingOperationTimeout,
        [this, &error, &operationDone, &sequenceNumber] (AsyncOperationSPtr const & operation)
    {
        NamePropertyMetadataResult metadataResult;
        error = propertyManagmentClient_->EndGetPropertyMetadata(operation, metadataResult);
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "GetServiceLocationSequenceNumber: SequenceNumber:{0}, Error:{1}",            
            metadataResult.SequenceNumber,
            error);

        if(error.IsSuccess())
        {
            sequenceNumber = metadataResult.SequenceNumber;
        }
        else
        {
            // If the property is not found, then complete with success
            if(error.IsError(ErrorCodeValue::PropertyNotFound))
            {
                // -1 indicates that sequence check should not be done
                sequenceNumber = -1;
                error = ErrorCodeValue::Success;
            }
        }

        operationDone.Set();
    },
        this->CreateAsyncOperationRoot());

    operationDone.WaitOne();

    return error;
}

ErrorCode FileStoreServiceReplica::RegisterServiceLocation(int64 const sequenceNumber)
{
    ErrorCode error;
    ManualResetEvent operationDone;
    
    // make a copy of the serialized data
    vector<BYTE> serializedData = serializedInfo_;

    NamingUri serviceName(this->serviceName_);
    NamePropertyOperationBatch propertyOperationBatch(serviceName);
    if(sequenceNumber > 0)
    {
        propertyOperationBatch.AddCheckSequenceOperation(this->PartitionId.ToString(), sequenceNumber);
    }
    else
    {
        propertyOperationBatch.AddCheckExistenceOperation(this->PartitionId.ToString(), false);
    }

    propertyOperationBatch.AddPutPropertyOperation(this->PartitionId.ToString(), move(serializedData), FABRIC_PROPERTY_TYPE_BINARY);

    propertyManagmentClient_->BeginSubmitPropertyBatch(
        move(propertyOperationBatch),
        FileStoreServiceConfig::GetConfig().NamingOperationTimeout,
        [this, &error, &operationDone] (AsyncOperationSPtr const & operation)
    {
        IPropertyBatchResultPtr result;
        error = propertyManagmentClient_->EndSubmitPropertyBatch(operation, result);
        if(error.IsSuccess())
        {
            error = result->GetError();
        }

        WriteInfo(
            TraceComponent,
            this->TraceId,
            "RegisterServiceLocation: Error:{0}",
            error);

        operationDone.Set();
    },
        this->CreateAsyncOperationRoot());
    
    operationDone.WaitOne();

    return error;
}

ErrorCode FileStoreServiceReplica::CreateName()
{     
    ManualResetEvent operationDone;
    ErrorCode error;

    propertyManagmentClient_->BeginCreateName(
        NamingUri(serviceName_),
        FileStoreServiceConfig::GetConfig().NamingOperationTimeout,
        [this, &error, &operationDone] (AsyncOperationSPtr const & operation)
    {
        error = propertyManagmentClient_->EndCreateName(operation);
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "CreateName: Error:{0}",
            error);
        operationDone.Set();
    },
        this->CreateAsyncOperationRoot());
    
    operationDone.WaitOne();
    
    if(!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameAlreadyExists))
    {
        return error;
    }

    return ErrorCodeValue::Success;
}

void FileStoreServiceReplica::DeleteStagingAndStoreFolder()
{
    if(Directory::Exists(this->StoreRoot))
    {
        auto error = Directory::Delete(this->StoreRoot, true, true);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "Unable to delete StoreRoot: {0}. Error:{1}",
                this->StoreRoot,
                error);        
        }
    }

    if(Directory::Exists(this->StagingRoot))
    {
        auto error = Directory::Delete(this->StagingRoot, true, true);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                this->TraceId,
                "Unable to delete StagingRoot: {0}. Error:{1}",
                this->StagingRoot,
                error);        
        }
    }   
}


#if !defined(PLATFORM_UNIX)

void FileStoreServiceReplica::CleanupFreeSpaceLoggingTimer()
{
    if (FileStoreServiceConfig::GetConfig().EnableLoggingOfDiskSpace)
    {
        AcquireWriteLock lock(freeSpaceLoggingtimerLock_);

        if (freeSpaceLoggingTimer_)
        {
            freeSpaceLoggingTimer_->Cancel();
            freeSpaceLoggingTimer_ = nullptr;
        }
    }
}

void FileStoreServiceReplica::LogFreeSpaceForDisks()
{
    // Taken and modified from https://msdn.microsoft.com/en-us/library/windows/desktop/cc542456(v=vs.85).aspx
    bool ret = false;
    DWORD  error = ERROR_SUCCESS, charCount = 0, sizeOfBuffer = MAX_PATH;
    WCHAR  deviceName[MAX_PATH] = L"", volumeName[MAX_PATH] = L"";

    HANDLE handle = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));
    if (handle == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        WriteNoise(
            TraceComponent,
            TraceId,
            "LogFreeSpaceForDisks Failed: Unable to get first volume {0}", error);
        return;
    }

    WCHAR nodeName[MAX_PATH];
    if (GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, nodeName, &sizeOfBuffer))
    {
        WriteNoise(
            TraceComponent,
            TraceId,
            "LogFreeSpaceForDisks : Node name: {0}", nodeName);
    }

    do
    {
        size_t index = wcslen(volumeName) - 1;
        if (volumeName[0] != L'\\' || volumeName[1] != L'\\' || volumeName[2] != L'?' ||
            volumeName[3] != L'\\' || volumeName[index] != L'\\')
        {
            error = ERROR_BAD_PATHNAME;
            WriteNoise(
                TraceComponent,
                TraceId,
                "LogFreeSpaceForDisks Failed: FindFirstVolumeW/FindNextVolumeW returned a bad path: {0}", volumeName);
            break;
        }

        volumeName[index] = L'\0';
        charCount = QueryDosDeviceW(&volumeName[4], deviceName, ARRAYSIZE(deviceName));
        volumeName[index] = L'\\';

        if (charCount == 0)
        {
            error = GetLastError();
            WriteNoise(
                TraceComponent,
                TraceId,
                "LogFreeSpaceForDisks Failed: QueryDosDevice failed with error code {0}", error);
            break;
        }

        WCHAR Names[MAX_PATH + 1];
        ret = GetVolumePathNamesForVolumeNameW(volumeName, Names, charCount, &charCount);
        if (!ret)
        {
            error = GetLastError();
            WriteNoise(
                TraceComponent,
                TraceId,
                "LogFreeSpaceForDisks Failed: GetVolumePathNamesForVolumeNameW failed with error code {0}", error);

            break;
        }

        ULARGE_INTEGER freeSpaceToUser, totalFreeSpace, totalSpace;
        WCHAR *pVolumeName = Names;
        if (!GetDiskFreeSpaceEx(pVolumeName, &freeSpaceToUser, &totalSpace, &totalFreeSpace))
        {
            error = GetLastError();
            WriteNoise(
                TraceComponent,
                TraceId,
                "LogFreeSpaceForDisks Failed: GetDiskFreeSpaceEx failed with error code {0}", error);

            // try with default root
            if (!GetDiskFreeSpaceEx(nullptr, &freeSpaceToUser, &totalSpace, &totalFreeSpace))
            {
                error = GetLastError();
                WriteInfo(
                    TraceComponent,
                    TraceId,
                    "LogFreeSpaceForDisks Failed: retrying of GetDiskFreeSpaceEx failed with error code {0}", error);
                break;
            }
        }

        WriteInfo(
            TraceComponent,
            TraceId,
            "LogFreeSpaceForDisks : volume name {0}, free space available to FSS {1}, total free space {2}, total space {3} Node name: {4}\n",
            Names, freeSpaceToUser.QuadPart, totalFreeSpace.QuadPart, totalSpace.QuadPart, nodeName);

        ret = FindNextVolumeW(handle, volumeName, ARRAYSIZE(volumeName));
    } while (ret && handle != INVALID_HANDLE_VALUE);

    FindVolumeClose(handle);
}
#endif
