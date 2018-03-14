// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Reliability;

    StringLiteral const TraceComponent("GetApplicationName");

     EntreeService::GetApplicationNameAsyncOperation::GetApplicationNameAsyncOperation(
        __in GatewayProperties & properties,
        ServiceModel::QueryArgumentMap const & queryArgs,
        ActivityId const & activityId,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : TimedAsyncOperation (timeout, callback, parent)
      , properties_(properties)
      , activityHeader_(activityId) 
      , reply_(nullptr)
    {
        queryArgs.TryGetValue(Query::QueryResourceProperties::Service::ServiceName, serviceNameStr_);
    }

    void EntreeService::GetApplicationNameAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (serviceNameStr_.empty()) {
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidNameUri));
            return;
        }

        NamingUri serviceName;
        bool success = NamingUri::TryParse(serviceNameStr_, serviceName);
        if (!success) {
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidNameUri));
            return;
        }

        auto operation = GetServiceDescriptionAsyncOperation::BeginGetCached(
            properties_,
            serviceName,
            activityHeader_,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnGetPsdComplete(operation, false); },
            thisSPtr);
        this->OnGetPsdComplete(operation, true);
    }

    void EntreeService::GetApplicationNameAsyncOperation::OnGetPsdComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        PartitionedServiceDescriptorSPtr psd;
        auto error = GetServiceDescriptionAsyncOperation::EndGetCached(operation, psd);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        NamingUri applicationName;
        bool success = true;
        if (psd->Service.Type.IsSystemServiceType())
        {
            success = NamingUri::TryParse(ServiceModel::SystemServiceApplicationNameHelper::SystemServiceApplicationName, applicationName);
        }
        else
        {
            success = NamingUri::TryParse(psd->Service.ApplicationName, applicationName);
        }
        if (!success)
        {
            this->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidNameUri));
            return;
        }

        unique_ptr<ServiceModel::ApplicationNameQueryResult> queryResultUPtr(new ServiceModel::ApplicationNameQueryResult(move(applicationName)));
        auto queryResult = ServiceModel::QueryResult(move(queryResultUPtr));

        reply_ = Common::make_unique<Transport::Message>(move(queryResult));
        this->TryComplete(thisSPtr, ErrorCode::Success());
    }

    ErrorCode EntreeService::GetApplicationNameAsyncOperation::End(AsyncOperationSPtr operation, MessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<GetApplicationNameAsyncOperation>(operation);
        auto error = casted->Error;
        
        if (error.IsSuccess())
        {
            reply = std::move(casted->reply_);
        }

        return error;
    }
}
