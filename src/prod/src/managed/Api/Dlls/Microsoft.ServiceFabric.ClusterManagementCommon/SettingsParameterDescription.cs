// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;

    public class SettingsParameterDescription
    {
        [Required(AllowEmptyStrings = false)]
        public string Name { get; set; }

        public string Value { get; set; }

        public SettingsTypeSectionParameter ToSettingsTypeSectionParameter()
        {
            return new SettingsTypeSectionParameter()
            {
                Name = this.Name,
                Value = this.Value,
                IsEncrypted = false,
                MustOverride = false
            };
        }
    }
}