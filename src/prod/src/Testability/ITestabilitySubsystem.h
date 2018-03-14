// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{   
    class ITestabilitySubsystem : 
        public Hosting2::ILegacyTestabilityRequestHandler
        , public Common::AsyncFabricComponent
    {
        DENY_COPY(ITestabilitySubsystem);

    public:
        ITestabilitySubsystem() = default;
        virtual ~ITestabilitySubsystem() = default;

        virtual Common::AsyncOperationSPtr BeginTestabilityOperation(
            Query::QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndTestabilityOperation(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr & reply) = 0;

        virtual void InitializeHealthReportingComponent(
            Client::HealthReportingComponentSPtr const & healthClient) = 0;
    };
}
