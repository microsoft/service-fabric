// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class QueryMetadataQueryResult
        : public Serialization::FabricSerializable
    {
    public:
        QueryMetadataQueryResult();

        QueryMetadataQueryResult(
            std::wstring const & queryName, 
            Common::StringCollection const & reqiuredArguments,
            Common::StringCollection const & optionalArguments);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM & publicQueryResultMetadata) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_QUERY_METADATA_QUERY_RESULT_ITEM const &publicApplicationQueryResult);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_03(queryName_, requiredArguments_, optionalArguments_)

    private:
        std::wstring queryName_;
        Common::StringCollection requiredArguments_;
        Common::StringCollection optionalArguments_;
    };

}
