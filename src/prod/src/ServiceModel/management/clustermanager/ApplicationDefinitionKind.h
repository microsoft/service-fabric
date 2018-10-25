// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace ApplicationDefinitionKind
        {
            enum class Enum
            {
                // We need to keep ServiceFabricApplicationDescription to 0 for backward compatibility
                Invalid = 0xFFFF,

                // Regular app - create application based on application type
                ServiceFabricApplicationDescription = 0,

                // For compose application - each application instance has its own type
                Compose= 1,

                // V2 app model - create application resource based on mesh description
                MeshApplicationDescription = 2,

            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            bool IsMatch(DWORD const publicValue, Enum const & value);

            FABRIC_APPLICATION_DEFINITION_KIND ToPublicApi(Enum const & toConvert);

            Enum FromPublicApi(FABRIC_APPLICATION_DEFINITION_KIND const toConvert);

            DECLARE_ENUM_STRUCTURED_TRACE( ApplicationDefinitionKind )
        }
    }
}
