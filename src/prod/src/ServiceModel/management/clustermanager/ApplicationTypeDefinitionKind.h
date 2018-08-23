// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace ApplicationTypeDefinitionKind
        {
            enum class Enum
            {
                Invalid = 0,

                // Regular type - create application based on application type
                ServiceFabricApplicationPackage = 1,
                // For compose application - each application instance has its own type
                Compose= 2,
                // For mesh application - each application instance has its own type
                MeshApplicationDescription = 3,
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            bool IsMatch(DWORD const publicValue, Enum const & value);

            FABRIC_APPLICATION_TYPE_DEFINITION_KIND ToPublicApi(Enum const & toConvert);

            Enum FromPublicApi(FABRIC_APPLICATION_TYPE_DEFINITION_KIND const toConvert);
        }
    }
}
