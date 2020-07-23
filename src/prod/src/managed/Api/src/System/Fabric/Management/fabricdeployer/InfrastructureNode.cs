// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Linq;

    internal class InfrastructureNode
    {
        public FabricNodeType FabricNode { get; private set; }
        public FabricEndpointsType NodeEndpoints { get; private set; }
        public CertificatesType NodeCertificates { get; private set; }

        public InfrastructureNode(FabricNodeType node, FabricEndpointsType endpoints, CertificatesType certificates)
        {
            FabricNode = node;
            NodeEndpoints = endpoints;
            NodeCertificates = certificates;
        }
    }
}