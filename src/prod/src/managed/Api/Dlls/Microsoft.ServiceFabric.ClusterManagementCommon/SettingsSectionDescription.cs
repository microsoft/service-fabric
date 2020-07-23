// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    public class SettingsSectionDescription
    {
        public SettingsSectionDescription()
        {
            this.Parameters = new List<SettingsParameterDescription>();
        }

        [Required(AllowEmptyStrings = false)]
        public string Name { get; set; }

        public List<SettingsParameterDescription> Parameters { get; set; }

        public SettingsTypeSection ToSettingsTypeSectionParameter()
        {
            return new SettingsTypeSection()
            {
                Name = this.Name,
                Parameter = this.Parameters.Select(parameter => parameter.ToSettingsTypeSectionParameter()).ToArray()
            };
        }
    }
}