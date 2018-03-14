// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetServiceNameQuerySpecificationHelper
        : public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
        DENY_COPY(GetServiceNameQuerySpecificationHelper)
    public:
        static std::wstring GetSpecificationId(ServiceModel::QueryArgumentMap const & queryArgs, Query::QueryNames::Enum const & queryName);
    private:
        static bool ShouldSendToFMM(ServiceModel::QueryArgumentMap const & queryArgs);
        static std::wstring GetInternalSpecificationId(bool shouldSendToFMM, Query::QueryNames::Enum const & queryName);
        std::vector<Query::QuerySpecificationSPtr> querySpecifications_;
    };
}
