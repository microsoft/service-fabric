// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ContainerServiceClient.Config
{
    using System.Collections.Generic;

    internal class ContainerEventsConfig
    {
        internal string Since { get; set; }

        internal string Until { get; set; }

        internal IDictionary<string, IList<string>> Filters { get; set; }
    }
}