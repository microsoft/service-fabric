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
                ClusterDefault = 1,
                CleanupApplicationPackageOnSuccessfulProvision = 2,
                NoCleanupApplicationPackageOnProvision = 3,
            };

            FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY ToPublicApi(Enum const&);
            Enum FromPublicApi(FABRIC_APPLICATION_PACKAGE_CLEANUP_POLICY toConvert);
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            DECLARE_ENUM_STRUCTURED_TRACE( ApplicationPackageCleanupPolicy )

            BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
                ADD_ENUM_VALUE(Invalid)
                ADD_ENUM_VALUE(ClusterDefault)
                ADD_ENUM_VALUE(CleanupApplicationPackageOnSuccessfulProvision)
                ADD_ENUM_VALUE(NoCleanupApplicationPackageOnProvision)
            END_DECLARE_ENUM_SERIALIZER()
        }
    }
}
