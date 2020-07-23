// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    internal sealed class ExtendedPropertyInfo
    {
        public DataType Type { get; set; }

        public string Name { get; set; }

        public Type RealType { get; set; }

        public int ManifestIndex { get; set; }

        public int ManifestVersion { get; set; }

        public string ManifestOriginalName { get; set; }
    }
}
