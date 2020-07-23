// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.ComponentModel.DataAnnotations;

    [Serializable]
    public class AzureActiveDirectory
    {
        [Required(AllowEmptyStrings = false)]
        public string TenantId { get; set; }

        [Required(AllowEmptyStrings = false)]
        public string ClusterApplication { get; set; }

        [Required(AllowEmptyStrings = false)]
        public string ClientApplication { get; set; }
    }
}