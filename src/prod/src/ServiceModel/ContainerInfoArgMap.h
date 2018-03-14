// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ContainerInfoArgMap: public Common::IFabricJsonSerializable
    {
    public:
        ContainerInfoArgMap();
        ~ContainerInfoArgMap();

        void Insert(std::wstring const & key, std::wstring const & value);

        std::wstring GetValue(std::wstring const & key);

        bool ContainsKey(std::wstring const & key);

        void Erase(std::wstring const & key);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_SIMPLE_MAP(L"ContainerInfoArgMap", containerInfoArgMap_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::map<std::wstring, std::wstring> containerInfoArgMap_;
    };
}
