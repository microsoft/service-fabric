// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class ReliableCollectionsRef
    {
        public ReliableCollectionsRef()
        {
            this.name = null;
            this.doNotPersistState = false;
        }

        public string name;
        public bool doNotPersistState;
    };
}

