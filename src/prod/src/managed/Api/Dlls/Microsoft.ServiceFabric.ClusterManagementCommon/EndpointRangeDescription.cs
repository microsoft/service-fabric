// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.ComponentModel.DataAnnotations;

    public class EndpointRangeDescription
    {
        [Required]
        public int StartPort { get; set; }

        [Required]
        public int EndPort { get; set; }
    }
}