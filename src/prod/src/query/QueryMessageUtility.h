// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryMessageUtility
        : protected Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public:
        class QueryMessageParseResult
        {
        public:
            enum Enum
            {
                // The query message is invalid and should be dropped
                Drop = 0,

                // The query message needs to be forwarded
                Forward = 1,

                // The query message is a parallel message
                Parallel = 2,

                // The query message is a message for a single query handler
                Single = 3,

                Sequential = 4,
            };
        };

        static QueryMessageParseResult::Enum ParseQueryMessage(
            Transport::Message & message,
            std::wstring const & ownerAddressSegment, // address segment of the owner
            std::wstring & addressSegment, // next address segment
            std::wstring & addressSegmentMetadata, // metadata out
            Common::ActivityId & activityId,
            QueryRequestMessageBodyInternal & messageBody);
    };
}
