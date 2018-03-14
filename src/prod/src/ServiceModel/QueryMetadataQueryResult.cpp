// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

QueryMetadataQueryResult::QueryMetadataQueryResult()
    : queryName_()
    , requiredArguments_()
    , optionalArguments_()
{
}

QueryMetadataQueryResult::QueryMetadataQueryResult(
    std::wstring const & queryName, 
    StringCollection const & requiredArguments,
    StringCollection const & optionalArguments)
    : queryName_(queryName)
    , requiredArguments_(requiredArguments)
    , optionalArguments_(optionalArguments)
{
}

void QueryMetadataQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM & publicQueryResultMetadata) const 
{
    publicQueryResultMetadata.QueryName = heap.AddString(queryName_);
    
    auto publicRequiredArguments = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, requiredArguments_, publicRequiredArguments);
    publicQueryResultMetadata.RequiredArguments = publicRequiredArguments.GetRawPointer();

    auto publicOptionalArguments = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, optionalArguments_, publicOptionalArguments);
    publicQueryResultMetadata.OptionalArguments = publicOptionalArguments.GetRawPointer();
}

ErrorCode QueryMetadataQueryResult::FromPublicApi(__in FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM const &publicApplicationQueryResult)
{
    queryName_ = wstring(publicApplicationQueryResult.QueryName);
    auto error = StringList::FromPublicApi(*publicApplicationQueryResult.RequiredArguments, requiredArguments_);
    if (error.IsSuccess())
    {
        error = StringList::FromPublicApi(*publicApplicationQueryResult.OptionalArguments, optionalArguments_);
    }

    return error;
}

void QueryMetadataQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("QueryName = {0}, ", queryName_);
    StringList::WriteTo(w, L"RequiredArguments", requiredArguments_);
    StringList::WriteTo(w, L"OptionalArguments", optionalArguments_);
}

std::wstring QueryMetadataQueryResult::ToString() const
{
    return wformatString(
        "QueryName = {0}, RequiredArguments = {1}, OptionalArguments = {2}",
        queryName_,
        requiredArguments_,
        optionalArguments_);
}
