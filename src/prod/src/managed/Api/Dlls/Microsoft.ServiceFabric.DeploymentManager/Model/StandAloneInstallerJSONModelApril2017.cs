// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Runtime.Serialization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;

    internal class StandAloneInstallerJSONModelApril2017 : StandAloneInstallerJSONModelJan2017
    {
        public const string April2017ModelApiVersion = "04-2017";

        /// <summary>
        /// JSON constructor.
        /// </summary>
        public StandAloneInstallerJSONModelApril2017()
        {
            this.Properties = new PropertyApril2017();
        }

        public StandAloneInstallerJSONModelApril2017(IUserConfig userConfig, IClusterTopology topology, string configurationVersion)
            : base(userConfig, topology, configurationVersion)
        {
            this.ApiVersion = StandAloneInstallerJSONModelApril2017.April2017ModelApiVersion;
            this.Properties = new PropertyApril2017();
            this.Properties.UpdateFromUserConfig(userConfig);
        }

        internal override void FromInternal(
            IClusterTopology clusterTopology,
            IUserConfig userConfig,
            string apiVersion)
        {
            base.FromInternal(clusterTopology, userConfig, apiVersion);
            this.Properties = new PropertyApril2017();
            this.Properties.UpdateFromUserConfig(userConfig);
        }
    }
}