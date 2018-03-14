// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NotificationClientConnectionRequestBody : public ServiceModel::ClientServerMessageBody
    {
        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        NotificationClientConnectionRequestBody() 
            : clientId_()
            , generation_()
            , clientVersions_()
            , filters_()
        { 
        }

        NotificationClientConnectionRequestBody(
            std::wstring const & clientId,
            Reliability::GenerationNumber const & generation,
            Common::VersionRangeCollectionSPtr && versions,
            std::vector<Reliability::ServiceNotificationFilterSPtr> && filters)
            : clientId_(clientId)
            , generation_(generation)
            , clientVersions_(std::move(versions))
            , filters_(std::move(filters))
        {
        }

        std::wstring const & GetClientId() { return clientId_; }
        Reliability::GenerationNumber const & GetGeneration() { return generation_; }
        Common::VersionRangeCollectionSPtr && TakeVersions() { return std::move(clientVersions_); }
        std::vector<Reliability::ServiceNotificationFilterSPtr> && TakeFilters() { return std::move(filters_); }

        FABRIC_FIELDS_04(clientId_, generation_, clientVersions_, filters_);
        
    private:
        std::wstring clientId_;
        Reliability::GenerationNumber generation_;
        Common::VersionRangeCollectionSPtr clientVersions_;
        std::vector<Reliability::ServiceNotificationFilterSPtr> filters_;
    };
}
