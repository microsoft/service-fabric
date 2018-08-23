// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Engine
{
    internal class EntitySynchronizer
    {
        public EntitySynchronizer()
        {
            this.NodeSynchronizer = new NodeCommandSynchronizer();
        }

        public NodeCommandSynchronizer NodeSynchronizer
        {
            get;
            private set;
        }
    }
}