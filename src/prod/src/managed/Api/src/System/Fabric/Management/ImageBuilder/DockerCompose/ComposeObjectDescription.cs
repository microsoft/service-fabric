// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.DockerCompose
{
    using System.Collections.Generic;
    using YamlDotNet.RepresentationModel;

    internal abstract class ComposeObjectDescription
    {
        public string Comments = null;
        public abstract void Parse(string traceContext, YamlNode node, HashSet<string> ignoredKeys);

        public abstract void Validate();
    }
}