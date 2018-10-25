// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "MockFabricClientImpl.h"

using namespace std;
using namespace ClientTest;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace ServiceModel;

//
// NOTE: Currently the methods of MockFabricClient just timeout after the time given in the api.
//

MockFabricClientImpl::MockFabricClientImpl(Common::FabricNodeConfigSPtr const& config)
    : config_(config)
{
}

#pragma region IServiceManagementClient Methods

//
// IServiceManagementClient methods
//

AsyncOperationSPtr MockFabricClientImpl::BeginCreateService(
    PartitionedServiceDescWrapper const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndCreateService(AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginInternalCreateService(
    PartitionedServiceDescriptor const &,
    ServicePackageVersion const &,
    ULONGLONG ,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);}

ErrorCode MockFabricClientImpl::EndInternalCreateService(Common::AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginCreateServiceFromTemplate(
    ServiceFromTemplateDescription const &,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndCreateServiceFromTemplate(
    AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginUpdateService(
    NamingUri const &,
    ServiceUpdateDescription const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndUpdateService(AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginDeleteService(
    DeleteServiceDescription const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndDeleteService(AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginGetServiceDescription(
    NamingUri const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndGetServiceDescription(
    AsyncOperationSPtr const & operation,
    __inout Naming::PartitionedServiceDescriptor & )
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginGetCachedServiceDescription(
    NamingUri const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndGetCachedServiceDescription(
    AsyncOperationSPtr const & operation,
    __inout Naming::PartitionedServiceDescriptor &)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginInternalGetServiceDescription(
    NamingUri const &,
    ActivityId const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndInternalGetServiceDescription(
    AsyncOperationSPtr const & operation,
    __inout Naming::PartitionedServiceDescriptor & )
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginResolveServicePartition(
    NamingUri const & ,
    ServiceResolutionRequestData const & ,
    Api::IResolvedServicePartitionResultPtr & ,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

AsyncOperationSPtr MockFabricClientImpl::BeginResolveServicePartition(
    NamingUri const &,
    PartitionKey const &,
    ResolvedServicePartitionMetadataSPtr const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndResolveServicePartition(
    AsyncOperationSPtr const & operation,
    __inout Api::IResolvedServicePartitionResultPtr &)
{
    return TimedAsyncOperation::End(operation);
}

ErrorCode MockFabricClientImpl::EndResolveServicePartition(
    AsyncOperationSPtr const & operation,
    __inout ResolvedServicePartitionSPtr &)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginResolveSystemServicePartition(
    NamingUri const &,
    PartitionKey const &,
    Guid const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndResolveSystemServicePartition(
    AsyncOperationSPtr const & operation,
    __inout ResolvedServicePartitionSPtr &)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginPrefixResolveServicePartition(
    NamingUri const & ,
    ServiceResolutionRequestData const & ,
    Api::IResolvedServicePartitionResultPtr & ,
    bool,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndPrefixResolveServicePartition(
    AsyncOperationSPtr const & operation,
    __inout ResolvedServicePartitionSPtr &)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginPrefixResolveServicePartitionInternal(
    NamingUri const &,
    ServiceResolutionRequestData const &,
    Api::IResolvedServicePartitionResultPtr &,
    bool,
    Guid const &,
    bool,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndPrefixResolveServicePartitionInternal(
    AsyncOperationSPtr const & operation,
    bool,
    __inout ResolvedServicePartitionSPtr &)
{
    return TimedAsyncOperation::End(operation);
}

ErrorCode MockFabricClientImpl::RegisterServicePartitionResolutionChangeHandler(
    NamingUri const&,
    PartitionKey const& ,
    Api::ServicePartitionResolutionChangeCallback const &,
    __inout LONGLONG * )
{
    return ErrorCodeValue::NotImplemented;
}

ErrorCode MockFabricClientImpl::UnRegisterServicePartitionResolutionChangeHandler(
    LONGLONG )
{
    return ErrorCodeValue::NotImplemented;
}

AsyncOperationSPtr MockFabricClientImpl::BeginGetServiceManifest(
    wstring const &,
    wstring const &,
    wstring const &,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndGetServiceManifest(
    AsyncOperationSPtr const &operation,
    __inout wstring &)
{
    return TimedAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr MockFabricClientImpl::BeginReportFault(
    std::wstring const & ,
    Common::Guid const & ,
    int64 ,
    Reliability::FaultType::Enum ,
    bool,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

Common::ErrorCode MockFabricClientImpl::EndReportFault(
    Common::AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr MockFabricClientImpl::BeginRegisterServiceNotificationFilter(
    Reliability::ServiceNotificationFilterSPtr const &,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

Common::ErrorCode MockFabricClientImpl::EndRegisterServiceNotificationFilter(
    Common::AsyncOperationSPtr const & operation,
    __out uint64 &)
{
    return TimedAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr MockFabricClientImpl::BeginUnregisterServiceNotificationFilter(
    uint64,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

Common::ErrorCode MockFabricClientImpl::EndUnregisterServiceNotificationFilter(
    Common::AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

AsyncOperationSPtr MockFabricClientImpl::BeginInternalDeleteService(
    DeleteServiceDescription const &,
    ActivityId const&,
    Common::TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

ErrorCode MockFabricClientImpl::EndInternalDeleteService(
    AsyncOperationSPtr const & operation,
    __inout bool & )
{
    return TimedAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr MockFabricClientImpl::BeginMovePrimary(
    std::wstring const &,
    Common::Guid const &,
    bool,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

Common::ErrorCode MockFabricClientImpl::EndMovePrimary(
    Common::AsyncOperationSPtr const & operation)
{
    return TimedAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr MockFabricClientImpl::BeginMoveSecondary(
    std::wstring const &,
    std::wstring const &,
    Common::Guid const &,
    bool,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<TimedAsyncOperation>(timeout, callback, parent);
}

Common::ErrorCode MockFabricClientImpl::EndMoveSecondary(
    Common::AsyncOperationSPtr const & operation){
    return TimedAsyncOperation::End(operation);
}

#pragma endregion
