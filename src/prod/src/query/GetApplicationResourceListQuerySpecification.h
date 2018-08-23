// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class GetApplicationResourceListQuerySpecification
        : public ParallelQuerySpecification
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
        DENY_COPY(GetApplicationResourceListQuerySpecification)

    public:
        GetApplicationResourceListQuerySpecification();

    protected:
        virtual Common::ErrorCode OnParallelQueryExecutionComplete(
            Common::ActivityId const & activityId,
            std::map<Query::QuerySpecificationSPtr, ServiceModel::QueryResult> & queryResults,
            __out Transport::MessageUPtr & replyMessage) override;
    };
}
