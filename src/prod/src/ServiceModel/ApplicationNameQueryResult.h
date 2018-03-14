// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationNameQueryResult 
        : public Serialization::FabricSerializable
    {
    public:
        ApplicationNameQueryResult();
        ApplicationNameQueryResult(Common::Uri const & applicationName);
        ApplicationNameQueryResult(ApplicationNameQueryResult const & other);
        ApplicationNameQueryResult(ApplicationNameQueryResult && other);

        __declspec(property(get=get_ApplicationName)) Common::Uri const & ApplicationName;
        Common::Uri const & get_ApplicationName() const { return applicationName_; }

        ApplicationNameQueryResult const & operator = (ApplicationNameQueryResult const & other);
        ApplicationNameQueryResult const & operator = (ApplicationNameQueryResult && other);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_APPLICATION_NAME_QUERY_RESULT & publicQueryResult) const;

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_NAME_QUERY_RESULT const& publicQueryResult);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_01(applicationName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::Uri applicationName_;
    };
}
