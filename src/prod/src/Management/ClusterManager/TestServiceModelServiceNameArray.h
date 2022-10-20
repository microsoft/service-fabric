// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class TestServiceModelServiceNames : public Serialization::FabricSerializable
        {
        public:
            FABRIC_FIELDS_01(ServiceNames);

        public:
            std::vector<ServiceModelServiceName> ServiceNames;
        };

        class TestServiceModelServiceNamesEx : public Serialization::FabricSerializable
        {
        public:
            FABRIC_FIELDS_01(ServiceNames);

        public:
            std::vector<ServiceModelServiceNameEx> ServiceNames;
        };
    }
}
