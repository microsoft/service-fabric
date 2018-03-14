// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryAddress
        : protected Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public:
        static Common::GlobalWString ReservedCharacters;

        QueryAddress(std::wstring const & queryAddress);
        Common::ErrorCode GetNextSegmentTo(std::wstring const & segment, _Out_ std::wstring & segmentAddress, _Out_ std::wstring & segmentAddressMetadata);
        
    private:
        Common::ErrorCode GetSegmentParts(std::wstring const & fullSegment, _Out_ std::wstring & segmentAddress, _Out_ std::wstring & segmentAddressMetadata);
        std::wstring queryAddress_;

    };
}
