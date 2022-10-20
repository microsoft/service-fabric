// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System.Collections.Generic;
    using System.Net;

    public class FabricNodeParameters
    {
        /// <summary>
        /// The id of this node
        /// </summary>
        public string NodeId { get; set; }

        /// <summary>
        /// The Name of this node
        /// </summary>
        public string NodeName { get; set; }

        /// <summary>
        /// The folder that contains all the files for this node
        /// </summary>
        public string NodeRootPath { get; set; }

        /// <summary>
        /// The URI for the image store
        /// </summary>
        public string ImageStoreConnectionString { get; set; }

        /// <summary>
        /// The path to the reliability folder
        /// </summary>
        public string ReliabilityPath { get; set; }

        /// <summary>
        /// The address of this node
        /// </summary>
        public IPEndPoint Address { get; set; }

        /// <summary>
        /// The voters
        /// </summary>
        public IEnumerable<IPEndPoint> Voters { get; set; }

        /// <summary>
        /// The start port for this node
        /// </summary>
        public int StartPort { get; set; }

        /// <summary>
        /// The end port for this node (inclusive)
        /// </summary>
        public int EndPort { get; set; }

        /// <summary>
        /// Fault domain ID for this node
        /// </summary>
        public Uri FaultDomainId { get; set; }

        /// <summary>
        /// Upgrade domain ID for this node
        /// </summary>
        public string UpgradeDomainId { get; set; }
    }
}