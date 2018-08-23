// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ClusterManager
    {
        namespace VolumeProvider
        {
            enum Enum
            {
                Invalid = 0,
                AzureFile = 1,
                ServiceFabricVolumeDisk = 2
            };

            void WriteToTextWriter(__in Common::TextWriter & w, Enum const & val);

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(AzureFile)
                ADD_ENUM_VALUE(ServiceFabricVolumeDisk)
            END_DECLARE_ENUM_SERIALIZER()
        };
    }
}
