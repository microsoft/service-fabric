// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class LoadValue
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        LoadValue(
            std::wstring const & name,
            uint value,
            Common::DateTime lastReportedTime);

        LoadValue();

        LoadValue(LoadValue && other);

        LoadValue & operator=(LoadValue && other);
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_LOAD_VALUE & publicResult) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_LOAD_VALUE const& publicResult);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_03(
            name_, 
            value_, 
            lastReportedTime_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, name_)
            SERIALIZABLE_PROPERTY(Constants::Value, value_)
            SERIALIZABLE_PROPERTY(Constants::LastReportedUtc, lastReportedTime_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring name_;
        uint value_;
        Common::DateTime lastReportedTime_;
    };

    DEFINE_USER_ARRAY_UTILITY(LoadValue);
}
