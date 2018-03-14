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
    class ApplicationNameQueryResultWrapper : public ApplicationNameQueryResult
    {
        DENY_COPY(ApplicationNameQueryResultWrapper)

    public:
        ApplicationNameQueryResultWrapper() 
            : ApplicationNameQueryResult()
            , applicationId_()
        {
        }

        ApplicationNameQueryResultWrapper(ServiceModel::ApplicationNameQueryResult && applicationNameQueryResult)
            : ApplicationNameQueryResult(std::move(applicationNameQueryResult))
        {
            Common::NamingUri::FabricNameToId(ApplicationName.ToString(), applicationId_);
        }

        ApplicationNameQueryResultWrapper(ServiceModel::ApplicationNameQueryResult && applicationNameQueryResult, bool useDelimiter)
            : ApplicationNameQueryResult(std::move(applicationNameQueryResult))
        {
            Common::NamingUri::FabricNameToId(ApplicationName.ToString(), useDelimiter, applicationId_);
        }

        ApplicationNameQueryResultWrapper(ApplicationNameQueryResultWrapper && other) 
            : ServiceModel::ApplicationNameQueryResult(std::move(other))
            , applicationId_(std::move(other.applicationId_))
        {
        }
        
        ApplicationNameQueryResultWrapper& operator = ( ApplicationNameQueryResultWrapper && other) 
        {
            if (this != &other)
            {
                applicationId_ = std::move(other.applicationId_);
            }

            ApplicationNameQueryResult::operator=(std::move(other));
            return *this;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Id, applicationId_)
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationId_;
    };
}
