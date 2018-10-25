// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;

    internal class FabricTestContext
    {
        private Dictionary<string, object> extensionProperties;

        public FabricTestContext(FabricClient fabricClient)
        {
            this.Random = new Random();
            this.ActionExecutor = new FabricTestActionExecutor(this);
            this.FabricClient = fabricClient;
            this.FabricCluster = new FabricCluster(this.FabricClient);
            this.extensionProperties = new Dictionary<string, object>();
        }

        public FabricTestActionExecutor ActionExecutor { get; private set; }

        public Random Random { get; private set; }

        public FabricCluster FabricCluster { get; private set; }

        public FabricClient FabricClient { get; private set; }

        public Dictionary<string, object> ExtensionProperties
        {
            get
            {
                return this.extensionProperties;
            }
        }
    }
}