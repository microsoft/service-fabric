// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // <ContainerLogDriver><DriverOpts> element.
    struct ContainerLogDriverConfigDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ContainerLogDriverConfigDescription();

        ContainerLogDriverConfigDescription(ContainerLogDriverConfigDescription const & other) = default;
        ContainerLogDriverConfigDescription(ContainerLogDriverConfigDescription && other) = default;

        ContainerLogDriverConfigDescription & operator = (ContainerLogDriverConfigDescription const & other) = default;
        ContainerLogDriverConfigDescription & operator = (ContainerLogDriverConfigDescription && other) = default;

        FABRIC_FIELDS_01(Config)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(L"Config", Config);
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::map<std::wstring, std::wstring> Config;
    };
}
