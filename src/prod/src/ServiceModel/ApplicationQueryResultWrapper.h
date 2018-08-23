// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    //
    // Application names are in URI format themselves. So this class changes the name to an ID
    // which can be used to identify this application instance in the REST URI.
    //
    class ApplicationQueryResultWrapper : public ApplicationQueryResult
    {
        DENY_COPY(ApplicationQueryResultWrapper)

    public:
        ApplicationQueryResultWrapper() 
            : ApplicationQueryResult()
            , applicationId_()
        {
        }

        ApplicationQueryResultWrapper(ServiceModel::ApplicationQueryResult && applicationQueryResult)
            : ApplicationQueryResult(std::move(applicationQueryResult))
        {
            Common::NamingUri::FabricNameToId(ApplicationName.ToString(), applicationId_);
        }

        ApplicationQueryResultWrapper(ServiceModel::ApplicationQueryResult && applicationQueryResult, bool useDelimiter)
            : ApplicationQueryResult(std::move(applicationQueryResult))
        {
            Common::NamingUri::FabricNameToId(ApplicationName.ToString(), useDelimiter, applicationId_);
        }

        ApplicationQueryResultWrapper(ApplicationQueryResultWrapper && other) 
            : ServiceModel::ApplicationQueryResult(std::move(other))
            , applicationId_(std::move(other.applicationId_))
        {
        }
        
        ApplicationQueryResultWrapper& operator = ( ApplicationQueryResultWrapper && other) 
        {
            if (this != &other)
            {
                applicationId_ = std::move(other.applicationId_);
            }

            ApplicationQueryResult::operator=(std::move(other));
            return *this;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::Id, applicationId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationId_;
    };

    // Used to serialize results in REST
    QUERY_JSON_LIST(ApplicationList, ApplicationQueryResultWrapper)
}
