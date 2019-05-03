//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once 

namespace Query
{
    class GetContainerCodePackageLogsQuerySpecification
        : public SequentialQuerySpecification
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public:
        explicit GetContainerCodePackageLogsQuerySpecification();

        virtual Common::ErrorCode GetNext(
            Query::QuerySpecificationSPtr const & previousQuerySpecification,
            __in std::unordered_map<Query::QueryNames::Enum, ServiceModel::QueryResult> & queryResults,
            __inout ServiceModel::QueryArgumentMap & queryArgs,
            __out Query::QuerySpecificationSPtr & nextQuery,
            __out Transport::MessageUPtr & replyMessage) override;

        std::wstring GetRelativeServiceName(std::wstring const & appName, std::wstring const & absoluteServiceName);
    };
}
