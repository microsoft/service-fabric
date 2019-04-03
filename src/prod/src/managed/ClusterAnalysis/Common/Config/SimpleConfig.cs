// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Config
{
    using System.Collections.Generic;

    public sealed class SimpleConfig
    {
        public string ConfigName { get; set; }

        public IDictionary<string, string> ConfigValues { get; set; }
    }
}