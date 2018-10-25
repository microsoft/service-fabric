//----------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;

    internal class ContainersPruneConfig
    {
        public IDictionary<string, IList<string>> Filters { get; set; }
    }
}
