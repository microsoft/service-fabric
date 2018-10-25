// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Management.ServiceModel;

    internal class CodePackage
    {
        public CodePackage(CodePackageType codePackageType)
        {
            this.CodePackageType = codePackageType;
        }

        public CodePackageType CodePackageType { get; set; }

        public string Checksum { get; set; }

        public string ConflictingChecksum { get; set; }
    }
}