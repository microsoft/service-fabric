// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ClusterManager
    {
        namespace ApplicationPackageCleanupPolicy
        {
            enum Enum
            {
                Invalid = 0,
                Default = 1,
                Automatic = 2,
                Manual = 3,
            };

            FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY ToPublicApi(Enum const&);
            Enum FromPublicApi(FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY toConvert);
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            DECLARE_ENUM_STRUCTURED_TRACE( ApplicationPackageCleanupPolicy )

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(Default)
                ADD_ENUM_VALUE(Automatic)
                ADD_ENUM_VALUE(Manual)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}
