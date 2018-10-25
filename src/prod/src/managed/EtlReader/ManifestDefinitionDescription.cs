// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System.Collections.Generic;

    public class ManifestDefinitionDescription
    {
        public ICollection<ProviderDefinitionDescription> Providers { get; set; }
    }
}