// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    //
    // Service names are in URI format themselves. So this class changes the name to an ID
    // which can be used to identify this service instance in the REST URI.
    //
    class ServiceNameQueryResultWrapper : public ServiceNameQueryResult
    {
        DENY_COPY(ServiceNameQueryResultWrapper)

    public:
        ServiceNameQueryResultWrapper() 
            : ServiceNameQueryResult()
            , serviceId_()
        {
        }

        ServiceNameQueryResultWrapper(ServiceModel::ServiceNameQueryResult && serviceNameQueryResult)
            : ServiceNameQueryResult(std::move(serviceNameQueryResult))
        {
            Common::NamingUri::FabricNameToId(ServiceName.ToString(), serviceId_);
        }

        ServiceNameQueryResultWrapper(ServiceModel::ServiceNameQueryResult && serviceNameQueryResult, bool useDelimiter)
            : ServiceNameQueryResult(std::move(serviceNameQueryResult))
        {
            Common::NamingUri::FabricNameToId(ServiceName.ToString(), useDelimiter, serviceId_);
        }

        ServiceNameQueryResultWrapper(ServiceNameQueryResultWrapper && other) 
            : ServiceModel::ServiceNameQueryResult(std::move(other))
            , serviceId_(std::move(other.serviceId_))
        {
        }
        
        ServiceNameQueryResultWrapper& operator = (ServiceNameQueryResultWrapper && other) 
        {
            if (this != &other)
            {
                serviceId_ = std::move(other.serviceId_);
            }

            ServiceNameQueryResult::operator=(std::move(other));
            return *this;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Id, serviceId_)
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceId_;
    };
}
