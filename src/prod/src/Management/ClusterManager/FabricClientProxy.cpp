// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Common::ErrorCodeValue;
using namespace std;
using namespace Store;
using namespace Naming;
using namespace Management::ClusterManager;
using namespace Api;

StringLiteral const TraceComponent("FabricClientProxy");

class FabricClientProxy::ServiceAsyncOperationBase 
    : public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>
    , public AsyncOperation 
{
public:
    ServiceAsyncOperationBase(
        __in FabricClientProxy const & owner,
        wstring const & serviceName,
        Common::ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ReplicaActivityTraceComponent(owner.PartitionedReplicaId, activityId)
        , AsyncOperation(callback, root)
        , owner_(owner)
        , serviceName_(serviceName)
        , timeoutHelper_(timeout)
    {
    }

protected:

    TimeSpan GetRemainingTime() { return timeoutHelper_.GetRemainingTime(); }

    virtual void OnGetExistingApplicationNameComplete(AsyncOperationSPtr const &, wstring const & existingApplicationName) = 0;

    void StartGetExistingApplicationName(AsyncOperationSPtr const & thisSPtr)
    {
        NamingUri uri;
        auto error = NamingUri::TryParse(serviceName_, this->TraceId, uri);

        if (error.IsSuccess())
        {
            // Use InternalGetServiceDescription to bypass service group check
            // that occurs on public GetServiceDescription API
            //
            auto operation = owner_.serviceMgmtClient_->BeginInternalGetServiceDescription(
                uri,
                this->ActivityId,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnGetServiceDescriptionComplete(operation, false); },
                thisSPtr);
            this->OnGetServiceDescriptionComplete(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnGetServiceDescriptionComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        PartitionedServiceDescriptor existingPsd;
        auto error = owner_.serviceMgmtClient_->EndInternalGetServiceDescription(operation, existingPsd);
        
        if (error.IsSuccess())
        {
            this->OnGetExistingApplicationNameComplete(thisSPtr, existingPsd.Service.ApplicationName);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

protected:
    FabricClientProxy const & owner_;

private:
    wstring serviceName_;
    TimeoutHelper timeoutHelper_;
};

class FabricClientProxy::CreateServiceAsyncOperation : public ServiceAsyncOperationBase
{
public:
    CreateServiceAsyncOperation(
        __in FabricClientProxy const & owner,
        PartitionedServiceDescriptor const & psd,
        ServiceModelVersion const & packageVersion,
        uint64 instanceId,
        Common::ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ServiceAsyncOperationBase(owner, psd.Service.Name, activityId, timeout, callback, root)
        , psd_(psd)
        , packageVersion_(packageVersion)
        , instanceId_(instanceId)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CreateServiceAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ServiceModel::ServicePackageVersion servicePackageVersion;
        ServiceModel::ServicePackageVersion::FromString(packageVersion_.Value, servicePackageVersion);

        auto operation = owner_.serviceMgmtClient_->BeginInternalCreateService(
            psd_,
            servicePackageVersion,
            instanceId_,
            this->GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnInternalCreateServiceComplete(operation, false); },
            thisSPtr);
        this->OnInternalCreateServiceComplete(operation, true);
    }

private:

    void OnInternalCreateServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_.serviceMgmtClient_->EndInternalCreateService(operation);

        if (error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
        {
            this->StartGetExistingApplicationName(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnGetExistingApplicationNameComplete(AsyncOperationSPtr const & thisSPtr, wstring const & existingApplicationName) override
    {
        if (existingApplicationName == psd_.Service.ApplicationName)
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::UserServiceAlreadyExists);
        }
        else
        {
            auto msg = wformatString(GET_RC( Mismatched_Application2 ),
                psd_.Service.Name,
                psd_.Service.ApplicationName,
                existingApplicationName);

            WriteWarning(TraceComponent, "{0}: {1}", this->TraceId, msg);

            ErrorCode error(ErrorCodeValue::InvalidNameUri, move(msg));

            this->TryComplete(thisSPtr, move(error));
        }
    }

    PartitionedServiceDescriptor psd_;
    ServiceModelVersion packageVersion_;
    uint64 instanceId_;
};

// *********************************************************************
// In V2, Naming service replicas will perform recursive name deletion
// when deleting a service. This functionality did not exist in V1, so
// this helper class attempts to detect if the delete service request
// was processed by a V1 replica and if so, perform recursive name
// deletion from the client.
// *********************************************************************

class FabricClientProxy::DeleteServiceAsyncOperation : public ServiceAsyncOperationBase
{
public: 
    DeleteServiceAsyncOperation(
        __in FabricClientProxy const & owner,
        NamingUri const & appName,
        ServiceModel::DeleteServiceDescription const & description,
        Common::ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ServiceAsyncOperationBase(owner, description.ServiceName.ToString(), activityId, timeout, callback, root)
        , appName_(appName)
        , description_(description)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<DeleteServiceAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartGetExistingApplicationName(thisSPtr);
    }

private:

    void OnGetExistingApplicationNameComplete(AsyncOperationSPtr const & thisSPtr, wstring const & existingApplicationName) override
    {
        if (existingApplicationName == appName_.ToString())
        {
            auto operation = owner_.serviceMgmtClient_->BeginInternalDeleteService(
                description_,
                this->ActivityId,
                this->GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnInternalDeleteServiceComplete(operation, false); },
                thisSPtr);
            this->OnInternalDeleteServiceComplete(operation, true);
        }
        else
        {
            // This should only happen during rollback of a service creation context where
            // the requested application name mismatched the application name of an
            // existing service.
            //
            this->WriteInfo(
                TraceComponent, 
                "{0}: application name mismatch - skipping delete service: service='{1}' expected app='{2}' actual app='{3}'",
                this->ActivityId,
                description_.ServiceName,
                appName_,
                existingApplicationName);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void OnInternalDeleteServiceComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        // Really old versions of Naming Service did not automatically cleanup implicit names when
        // deleting a service. This flag was used to determine if the cleanup might not have
        // occurred so that an explicit delete name request could be sent.
        //
        bool unusedNameDeleted;
        auto error = owner_.serviceMgmtClient_->EndInternalDeleteService(operation, unusedNameDeleted);

        this->TryComplete(thisSPtr, error);
    }

    NamingUri appName_;
    ServiceModel::DeleteServiceDescription description_;
};

//
// FabricClientProxy API
//

FabricClientProxy::FabricClientProxy(
    Api::IClientFactoryPtr const & clientFactory, Store::PartitionedReplicaId const & partitionedReplicaId)
    : PartitionedReplicaTraceComponent(partitionedReplicaId)
    , clientFactory_(clientFactory)
{
}

ErrorCode FabricClientProxy::Initialize()
{
    auto error = clientFactory_->CreateClusterManagementClient(clusterMgmtClient_);
    if (!error.IsSuccess()) { return error; }

    error = clientFactory_->CreateInternalInfrastructureServiceClient(infrastructureClient_);
    if (!error.IsSuccess()) { return error; }

    error = clientFactory_->CreatePropertyManagementClient(propertyMgmtClient_);
    if (!error.IsSuccess()) { return error; }

    error = clientFactory_->CreateApplicationManagementClient(applicationManagementClient_);
    if (!error.IsSuccess()) { return error; }

    return clientFactory_->CreateServiceManagementClient(serviceMgmtClient_);
}

AsyncOperationSPtr FabricClientProxy::BeginCreateName(
    NamingUri const & name,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerCreateName(ReplicaActivityId(this->PartitionedReplicaId, activityId), name.ToString());

    return propertyMgmtClient_->BeginCreateName(
        name,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndCreateName(AsyncOperationSPtr const & operation) const
{
    return propertyMgmtClient_->EndCreateName(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginDeleteName(
    NamingUri const & name,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerDeleteName(ReplicaActivityId(this->PartitionedReplicaId, activityId), name.ToString());

    return propertyMgmtClient_->BeginDeleteName(
        name,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndDeleteName(AsyncOperationSPtr const & operation) const
{
    auto error = propertyMgmtClient_->EndDeleteName(operation);

    //
    // Cases where the name was already deleted, or explicitly created and has some properties set,
    // are treated as success. 
    //
    if (error.IsError(ErrorCodeValue::NameNotFound) ||
        error.IsError(ErrorCodeValue::NameNotEmpty) ||
        error.IsError(ErrorCodeValue::AccessDenied) ||
        error.IsError(ErrorCodeValue::InvalidNameUri))
    {
        error = ErrorCodeValue::Success;
    }

    return error;
}

Common::AsyncOperationSPtr FabricClientProxy::BeginGetProperty(
    Common::NamingUri const & name,
    std::wstring const & propertyName,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) const
{
    return propertyMgmtClient_->BeginGetProperty(name, propertyName, timeout, callback, parent);
}

Common::ErrorCode FabricClientProxy::EndGetProperty(
    Common::AsyncOperationSPtr const & operation,
    __inout Naming::NamePropertyResult & result) const
{
    return propertyMgmtClient_->EndGetProperty(operation, result);
}

Common::AsyncOperationSPtr FabricClientProxy::BeginSubmitPropertyBatch(
    Naming::NamePropertyOperationBatch && batch,
    ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerSubmitPropertyBatch(ReplicaActivityId(this->PartitionedReplicaId, activityId), batch.ToString());

    return propertyMgmtClient_->BeginSubmitPropertyBatch(
        move(batch),
        timeout,
        callback,
        root);
}

Common::ErrorCode FabricClientProxy::EndSubmitPropertyBatch(
    Common::AsyncOperationSPtr const & operation,
    __inout IPropertyBatchResultPtr & result) const
{
    return propertyMgmtClient_->EndSubmitPropertyBatch(operation, result);
}

AsyncOperationSPtr FabricClientProxy::BeginCreateService(
    PartitionedServiceDescriptor const & psd,
    ServiceModelVersion const & packageVersion,
    uint64 instanceId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerCreateService(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        psd.Service.Name,
        packageVersion.Value,
        instanceId);

    return AsyncOperation::CreateAndStart<CreateServiceAsyncOperation>(
        *this,
        psd,
        packageVersion,
        instanceId,
        activityId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndCreateService(AsyncOperationSPtr const & operation) const
{
    return CreateServiceAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginUpdateService(
    NamingUri const & serviceName,
    ServiceUpdateDescription const & updateDescription,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerUpdateService(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        serviceName,
        updateDescription);

    return serviceMgmtClient_->BeginUpdateService(
        serviceName,
        updateDescription,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndUpdateService(AsyncOperationSPtr const & operation) const
{
    return serviceMgmtClient_->EndUpdateService(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginDeleteService(
    NamingUri const & appName,
    ServiceModel::DeleteServiceDescription const & description,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerDeleteService(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        description); 

    return AsyncOperation::CreateAndStart<DeleteServiceAsyncOperation>(
        *this,
        appName,
        description,
        activityId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndDeleteService(AsyncOperationSPtr const & operation) const
{
    return DeleteServiceAsyncOperation::End(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginGetServiceDescription(
    NamingUri const & serviceName,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerGetServiceDescription(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        serviceName);

    return serviceMgmtClient_->BeginGetServiceDescription(
        serviceName,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndGetServiceDescription(
    AsyncOperationSPtr const & operation,
    __out Naming::PartitionedServiceDescriptor & description) const
{
    return serviceMgmtClient_->EndGetServiceDescription(
        operation,
        description);
}

AsyncOperationSPtr FabricClientProxy::BeginPutProperty(
    NamingUri const & name,
    wstring const & propertyName,
    vector<byte> const & data,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerPutProperty(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        name.ToString(),
        propertyName);

    vector<byte> temp = data;
    return propertyMgmtClient_->BeginPutProperty(
        name,
        propertyName,
        FABRIC_PROPERTY_TYPE_BINARY,
        move(temp),
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndPutProperty(AsyncOperationSPtr const & operation) const
{
    return propertyMgmtClient_->EndPutProperty(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginDeleteProperty(
    NamingUri const & name,
    wstring const & propertyName,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerDeleteProperty(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        name.ToString(),
        propertyName);

    return propertyMgmtClient_->BeginDeleteProperty(
        name,
        propertyName,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndDeleteProperty(AsyncOperationSPtr const & operation) const
{
    return propertyMgmtClient_->EndDeleteProperty(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginDeactivateNodesBatch(
    map<Federation::NodeId, Reliability::NodeDeactivationIntent::Enum> const & deactivations,
    wstring const & batchId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerDeactivateNodesBatch(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        batchId);

    return clusterMgmtClient_->BeginDeactivateNodesBatch(
        deactivations,
        batchId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndDeactivateNodesBatch(AsyncOperationSPtr const & operation) const
{
    return clusterMgmtClient_->EndDeactivateNodesBatch(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginRemoveNodeDeactivations(
    wstring const & batchId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerRemoveNodeDeactivations(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        batchId);

    return clusterMgmtClient_->BeginRemoveNodeDeactivations(
        batchId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndRemoveNodeDeactivations(AsyncOperationSPtr const & operation) const
{
    return clusterMgmtClient_->EndRemoveNodeDeactivations(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginGetNodeDeactivationStatus(
    wstring const & batchId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerGetNodeDeactivationStatus(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        batchId);

    return clusterMgmtClient_->BeginGetNodeDeactivationStatus(
        batchId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndGetNodeDeactivationStatus(
    AsyncOperationSPtr const & operation,
    __out Reliability::NodeDeactivationStatus::Enum & status) const
{
    return clusterMgmtClient_->EndGetNodeDeactivationStatus(operation, status);
}

AsyncOperationSPtr FabricClientProxy::BeginNodeStateRemoved(
    wstring const & nodeName,
    Federation::NodeId const& nodeId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerNodeStateRemoved(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        nodeName,
        nodeId);

    return clusterMgmtClient_->BeginNodeStateRemoved(
        nodeName,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndNodeStateRemoved(AsyncOperationSPtr const & operation) const
{
    return clusterMgmtClient_->EndNodeStateRemoved(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginReportStartTaskSuccess(
    TaskInstance const & taskInstance,
    Guid const & targetPartitionId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerReportStartTaskSuccess(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        taskInstance);
    
    return infrastructureClient_->BeginReportStartTaskSuccess(
        taskInstance.Id,
        taskInstance.Instance,
        targetPartitionId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndReportStartTaskSuccess(Common::AsyncOperationSPtr const & operation) const
{
    return infrastructureClient_->EndReportStartTaskSuccess(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginReportFinishTaskSuccess(
    TaskInstance const & taskInstance,
    Guid const & targetPartitionId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerReportFinishTaskSuccess(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        taskInstance);
    
    return infrastructureClient_->BeginReportFinishTaskSuccess(
        taskInstance.Id,
        taskInstance.Instance,
        targetPartitionId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndReportFinishTaskSuccess(Common::AsyncOperationSPtr const & operation) const
{
    return infrastructureClient_->EndReportFinishTaskSuccess(operation);
}

AsyncOperationSPtr FabricClientProxy::BeginReportTaskFailure(
    TaskInstance const & taskInstance,
    Guid const & targetPartitionId,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root) const
{
    CMEvents::Trace->InnerReportTaskFailure(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        taskInstance);
    
    return infrastructureClient_->BeginReportTaskFailure(
        taskInstance.Id,
        taskInstance.Instance,
        targetPartitionId,
        timeout,
        callback,
        root);
}

ErrorCode FabricClientProxy::EndReportTaskFailure(Common::AsyncOperationSPtr const & operation) const
{
    return infrastructureClient_->EndReportTaskFailure(operation);
}

Common::AsyncOperationSPtr Management::ClusterManager::FabricClientProxy::BeginUnprovisionApplicationType(
    Management::ClusterManager::UnprovisionApplicationTypeDescription const & description, 
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const & callback, 
    Common::AsyncOperationSPtr const &root) const
{
    return applicationManagementClient_->BeginUnprovisionApplicationType(
        description,
        timeout,
        callback,
        root);
}

Common::ErrorCode Management::ClusterManager::FabricClientProxy::EndUnprovisionApplicationType(Common::AsyncOperationSPtr const &operation) const
{
    return applicationManagementClient_->EndUnprovisionApplicationType(operation);
}
