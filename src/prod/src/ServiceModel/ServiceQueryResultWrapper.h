// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    //
    // Service names are in URI format themselves. So this class changes the name to an ID
    // which can be used to identify this service in the REST URI.
    //
    class ServiceQueryResultWrapper : public ServiceQueryResult
    {
        DENY_COPY(ServiceQueryResultWrapper)

    public:
        ServiceQueryResultWrapper()
            : ServiceQueryResult()
            , serviceId_()
        {
        }

        ServiceQueryResultWrapper(ServiceModel::ServiceQueryResult &queryResult)
            : ServiceQueryResult(std::move(queryResult))
        {
            Common::NamingUri::FabricNameToId(ServiceName.ToString(), serviceId_);
        }

        ServiceQueryResultWrapper(ServiceModel::ServiceQueryResult &queryResult, bool useDelimiter)
            : ServiceQueryResult(std::move(queryResult))
        {
            Common::NamingUri::FabricNameToId(ServiceName.ToString(), useDelimiter, serviceId_);
        }

        ServiceQueryResultWrapper(ServiceQueryResultWrapper &&other)
            : ServiceQueryResult(std::move(other))
            , serviceId_(std::move(other.serviceId_))
        {
        }

        ServiceQueryResultWrapper & operator = (ServiceQueryResultWrapper &&other)
        {
            if (this != &other)
            {
                serviceId_ = std::move(other.serviceId_);
            }

            ServiceQueryResult::operator=(std::move(other));
            return *this;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Id, serviceId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceId_;
    };
    
    // Used to serialize results in REST
    QUERY_JSON_LIST(ServiceList, ServiceQueryResultWrapper)
}
