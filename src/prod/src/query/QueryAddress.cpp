// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;

Common::GlobalWString QueryAddress::ReservedCharacters = Common::make_global<std::wstring>(L"/[]");
static const StringLiteral TraceType("QueryAddress");

QueryAddress::QueryAddress(wstring const & queryAddress)
    : queryAddress_(queryAddress)
{
}

ErrorCode QueryAddress::GetNextSegmentTo(wstring const & segment, _Out_ wstring & segmentAddress, _Out_ wstring & segmentAddressMetadata)
{
    vector<wstring> tokens;
    StringUtility::Split<wstring>(queryAddress_, tokens, L"/");    

    if (tokens.size() == 0 && segment == L"/" && queryAddress_ == L"/")
    {
        segmentAddress = L".";    
        segmentAddressMetadata = L"";
        return ErrorCodeValue::Success;
    }
    else if(tokens.size() > 0 && segment == L"/")
    {
        return GetSegmentParts(tokens[0], segmentAddress, segmentAddressMetadata);
    }
    else if(tokens.size() == 0)
    {
        WriteWarning(
            TraceType,
            "Invalid address format: {0}", 
            queryAddress_);

        return ErrorCodeValue::InvalidAddress;
    }

    size_t tokenPosition = 0;
    for (auto token : tokens)
    {
        auto error = GetSegmentParts(token, segmentAddress, segmentAddressMetadata);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (StringUtility::AreEqualCaseInsensitive(segmentAddress, segment))
        {
            if (tokenPosition + 1 == tokens.size())
            {
                // "." indicated end of address
                segmentAddress = L".";
                segmentAddressMetadata = L"";
                return ErrorCodeValue::Success;
            }

            return GetSegmentParts(tokens[tokenPosition + 1], segmentAddress, segmentAddressMetadata);
        }

        tokenPosition++;
    }

    WriteWarning(
        TraceType,
        "Owner's segment {0} not found. Invalid address format",
        segment);
    return ErrorCodeValue::InvalidAddress;
}

ErrorCode QueryAddress::GetSegmentParts(std::wstring const & fullSegment, _Out_ std::wstring & segmentAddress, _Out_ std::wstring & segmentAddressMetadata)
{
    // We are validating that there are no more than one "[" and one "]" in a segment.        
    auto metadataStart = fullSegment.find_first_of('[');
    if (metadataStart == wstring::npos)
    {
        if (fullSegment.find_first_of(']') != wstring::npos)
        {
            WriteWarning(
                TraceType,
                "Invalid address format. Found ']' without starting '[' - FullSegment: {0}",
                fullSegment);
            return ErrorCodeValue::InvalidAddress;
        }

        segmentAddress = fullSegment;
        segmentAddressMetadata = L"";
    }
    else
    {
        // In the metadata part (i.e. section starting from first '[') make sure that there are no more '[' or ']'
        if (fullSegment.find_first_of('[', metadataStart + 1) != wstring::npos
            || fullSegment.find_first_of(L']') != fullSegment.size() - 1)
        {
            WriteWarning(
                TraceType,
                "Invalid address format. Segment metadata should only have one '[' and only one ']'. ']' should be at the end of metadata - FullSegment: {0}",
                fullSegment);
            return ErrorCodeValue::InvalidAddress;
        }

        segmentAddress = fullSegment.substr(0, metadataStart);
        segmentAddressMetadata = fullSegment.substr(metadataStart + 1, fullSegment.size() - metadataStart - 2);
    }

    return ErrorCodeValue::Success;
}
