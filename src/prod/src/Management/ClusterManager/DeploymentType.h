//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        namespace DeploymentType
        {
            enum class Enum
            {
                Invalid = 0,
                Compose = 1, // Reserved
                V2Application = 2 // TODO: Finalize names
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        }
    }
}
