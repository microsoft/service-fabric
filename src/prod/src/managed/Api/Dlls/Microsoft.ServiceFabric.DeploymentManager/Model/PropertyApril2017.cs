// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Runtime.Serialization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal class PropertyApril2017 : PropertyJanuary2017
    {
        public PropertyApril2017()
        {
            this.AddonFeatures = new List<AddonFeature>();
        }

        [JsonProperty(ItemConverterType = typeof(StringEnumConverter))]
        public List<AddonFeature> AddonFeatures { get; set; }

        [OnDeserialized]
        internal void OnDeserializedCallback(StreamingContext context)
        {
            if (this.AddonFeatures == null)
            {
                this.AddonFeatures = new List<AddonFeature>();
            }
        }

        internal override void UpdateUserConfig(IUserConfig userConfig, List<NodeDescriptionGA> nodes)
        {
            base.UpdateUserConfig(userConfig, nodes);
            userConfig.AddonFeatures = this.AddonFeatures;
        }

        internal override void UpdateFromUserConfig(IUserConfig userConfig)
        {
            base.UpdateFromUserConfig(userConfig);

            if (userConfig.AddonFeatures != null)
            {
                this.AddonFeatures = userConfig.AddonFeatures;
            }
        }
    }
}