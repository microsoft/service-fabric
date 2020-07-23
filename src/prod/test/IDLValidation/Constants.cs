// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace System.Fabric.Setup
{
    internal static class Constants
    {       
        public const string FabricDropPath = @"%_NTTREE%\FabricDrop";

        public const string IncDirectory = @"SDK\inc";
        public const string InternalIncDirectory = @"SDK.internal\inc.internal";
        public const string UnitTestDirectory = @"%_NTTREE%\FabricUnitTests";
        public const string ReleaseFileDirectory = @"IDLFiles";
    }
}