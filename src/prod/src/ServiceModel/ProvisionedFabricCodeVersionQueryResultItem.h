// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ProvisionedFabricCodeVersionQueryResultItem 
        : public Serialization::FabricSerializable
    {
    public:
        ProvisionedFabricCodeVersionQueryResultItem();
        ProvisionedFabricCodeVersionQueryResultItem(std::wstring const & codeVersion);

        ProvisionedFabricCodeVersionQueryResultItem(ProvisionedFabricCodeVersionQueryResultItem const & other) = default;
        ProvisionedFabricCodeVersionQueryResultItem(ProvisionedFabricCodeVersionQueryResultItem && other) = default;
        ProvisionedFabricCodeVersionQueryResultItem & operator = (ProvisionedFabricCodeVersionQueryResultItem const & other) = default;
        ProvisionedFabricCodeVersionQueryResultItem & operator = (ProvisionedFabricCodeVersionQueryResultItem && other) = default;

        __declspec(property(get=get_CodeVersion)) std::wstring const & CodeVersion;
        std::wstring const & get_CodeVersion() const { return codeVersion_; }
                
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_ITEM & publicQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_01(codeVersion_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::CodeVersion, codeVersion_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring codeVersion_;
    };
}
