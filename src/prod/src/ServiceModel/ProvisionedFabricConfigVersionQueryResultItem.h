// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ProvisionedFabricConfigVersionQueryResultItem 
        : public Serialization::FabricSerializable
    {
    public:
        ProvisionedFabricConfigVersionQueryResultItem();
        ProvisionedFabricConfigVersionQueryResultItem(std::wstring const & configVersion);
        
        ProvisionedFabricConfigVersionQueryResultItem(ProvisionedFabricConfigVersionQueryResultItem const & other) = default;
        ProvisionedFabricConfigVersionQueryResultItem(ProvisionedFabricConfigVersionQueryResultItem && other) = default;

        ProvisionedFabricConfigVersionQueryResultItem & operator = (ProvisionedFabricConfigVersionQueryResultItem const & other) = default;
        ProvisionedFabricConfigVersionQueryResultItem & operator = (ProvisionedFabricConfigVersionQueryResultItem && other) = default;

        __declspec(property(get=get_ConfigVersion)) std::wstring const & ConfigVersion;
        std::wstring const & get_ConfigVersion() const { return configVersion_; }
        
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_ITEM & publicQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_01(configVersion_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ConfigVersion, configVersion_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring configVersion_;
    };
}
