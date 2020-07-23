// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreConnection
{
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Connection information for connecting to local store.
    /// </summary>
    public class LocalTraceStoreConnectionInformation : TraceStoreConnectionInformation
    {
        /// <summary>
        /// </summary>
        /// <param name="configuration"></param>
        public LocalTraceStoreConnectionInformation(string fabricRoot, string logRoot, string codePath)
        {
            // TODO : Is there a legit case where one of these values can be null/empty?
            Assert.IsNotEmptyOrNull(fabricRoot, "Fabric Root can't be null/empty");
            Assert.IsNotEmptyOrNull(logRoot, "Fabric Log Root can't be null/empty");
            Assert.IsNotEmptyOrNull(codePath, "Fabric Code Path can't be null/empty");

            this.FabricDataRoot = fabricRoot;
            this.FabricLogRoot = logRoot;
            this.FabricCodePath = codePath;
        }

        /// <summary>
        /// FabricDataRoot
        /// </summary>
        internal string FabricDataRoot { get; private set; }

        /// <summary>
        /// FabricLogRoot
        /// </summary>
        internal string FabricLogRoot { get; private set; }

        /// <summary>
        /// FabricCodePath
        /// </summary>
        internal string FabricCodePath { get; private set; }
    }
}
