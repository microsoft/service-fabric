// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    public class ServiceEtwManifestInfo
    {
        internal string DataPackageName { get; set; }

        internal Version DataPackageVersion { get; set; }

        internal string Path { get; set; }

        public override bool Equals(object obj)
        {
            ServiceEtwManifestInfo manifestInfo = (ServiceEtwManifestInfo)obj;
            return manifestInfo.Path.Equals(this.Path, StringComparison.Ordinal);
        }

        public override int GetHashCode()
        {
            return this.Path.GetHashCode();
        }
    }
}