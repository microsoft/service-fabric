// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;

    internal class StandAloneInstallerJSONModelMay2017 : StandAloneInstallerJsonModelBase
    {
        public const string ModelApiVersion = "05-2017";

        /// <summary>
        /// JSON constructor.
        /// </summary>
        public StandAloneInstallerJSONModelMay2017()
        {
        }

        public StandAloneInstallerJSONModelMay2017(IUserConfig config, IClusterTopology topology, string configurationVersion)
            : base(config, topology, configurationVersion)
        {
            this.ApiVersion = StandAloneInstallerJSONModelMay2017.ModelApiVersion;
            this.Properties = new PropertyMay2017();
            this.Properties.UpdateFromUserConfig(config);
        }

        public PropertyMay2017 Properties
        {
            get;
            set;
        }

        internal override string GetClusterRegistrationId()
        {
            return this.Properties.ClusterRegistrationId;
        }

        internal override Dictionary<string, string> GetFabricSystemSettings()
        {
            return this.Properties.GetFabricSystemSettings();
        }

        internal override void FromInternal(
            IClusterTopology clusterTopology,
            IUserConfig userConfig,
            string apiVersion)
        {
            base.FromInternal(clusterTopology, userConfig, apiVersion);
            this.Properties = new PropertyMay2017();
            this.Properties.UpdateFromUserConfig(userConfig);
        }

        internal override void ValidateModel()
        {
            base.ValidateModel();
            this.Properties.ThrowValidationExceptionIfNull("Properties");
            this.Properties.Validate(this.Nodes);
        }

        protected override StandAloneUserConfig OnGetUserConfig()
        {
            var userConfig = new StandAloneUserConfig();
            this.UpdateUserConfig(userConfig);
            this.Properties.UpdateUserConfig(userConfig, this.Nodes);
            return userConfig;
        }
    }
}