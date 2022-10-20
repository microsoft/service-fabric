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
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal class PropertyJune2018 : PropertyOctober2017
    {
        public PropertyJune2018()
        {
        }

        public string CodeVersion
        {
            get;
            set;
        }

        public int GoalStateExpirationReminderInDays
        {
            get;
            set;
        }

        internal override void Validate(List<NodeDescriptionGA> nodes)
        {
            base.Validate(nodes);
        }

        internal override void UpdateUserConfig(IUserConfig userConfig, List<NodeDescriptionGA> nodes)
        {
            base.UpdateUserConfig(userConfig, nodes);
            userConfig.CodeVersion = this.CodeVersion;
            userConfig.GoalStateExpirationReminderInDays = this.GoalStateExpirationReminderInDays;
        }

        internal override void UpdateFromUserConfig(IUserConfig userConfig)
        {
            base.UpdateFromUserConfig(userConfig);
            this.CodeVersion = userConfig.CodeVersion;
            this.GoalStateExpirationReminderInDays = userConfig.GoalStateExpirationReminderInDays;
        }
    }
}