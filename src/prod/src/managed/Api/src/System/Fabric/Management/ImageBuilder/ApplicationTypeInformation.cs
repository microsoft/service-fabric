// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    internal class ApplicationTypeInformation
    {
        public ApplicationTypeInformation(string name, string version)
        {
            this.Name = name;
            this.Version = version;
        }

        public string Name { get; private set; }

        public string Version { get; private set; }
    }
}