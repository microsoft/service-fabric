// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using Newtonsoft.Json;

    /// <summary>
    /// The output & internal state describing a cluster and the input on which 
    /// the output is based on.
    /// </summary>
    [JsonObject(IsReference = true)]
    internal class ClusterState
    {
        [JsonConstructor]
        public ClusterState(IUserConfig csmConfig, IAdminConfig wrpConfig, ClusterNodeConfig nodeConfig, ClusterExternalState externalState)
        {
            this.CSMConfig = csmConfig;
            this.WRPConfig = wrpConfig;
            this.NodeConfig = nodeConfig;
            this.ExternalState = externalState;
        }

        public ClusterExternalState ExternalState { get; set; }

        /// <summary>
        /// The CSM input from which this output is generated.
        /// </summary>
        public IUserConfig CSMConfig { get; set; }

        /// <summary>
        /// The WRP input from which the output is generated.
        /// </summary>
        public IAdminConfig WRPConfig { get; set; }

        public ClusterNodeConfig NodeConfig { get; set; }        
    }
}