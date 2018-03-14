// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // Application names are in URI format themselves. So this class changes the name to an ID
    // which can be used to identify this application instance in the REST URI.
    //
    class DeployedApplicationQueryResultWrapper : public ServiceModel::DeployedApplicationQueryResult
    {
    public:
        DeployedApplicationQueryResultWrapper() = default;

        DeployedApplicationQueryResultWrapper(ServiceModel::DeployedApplicationQueryResult & applicationQueryResult)
            : DeployedApplicationQueryResult(std::move(applicationQueryResult))
        {
            Common::NamingUri::FabricNameToId(ApplicationName, applicationId_);
        }

        DeployedApplicationQueryResultWrapper(DeployedApplicationQueryResultWrapper && other) = default;

        DeployedApplicationQueryResultWrapper(ServiceModel::DeployedApplicationQueryResult & applicationQueryResult, bool useDelimiter)
            : DeployedApplicationQueryResult(std::move(applicationQueryResult))
        {
            Common::NamingUri::FabricNameToId(ApplicationName, useDelimiter, applicationId_);
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Id, applicationId_)
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationId_;
    };

    // Used to serialize results in REST
    // This is a macro that defines type DeployedApplicationPagedListWrapper
    QUERY_JSON_LIST(DeployedApplicationPagedListWrapper, DeployedApplicationQueryResultWrapper)
}
