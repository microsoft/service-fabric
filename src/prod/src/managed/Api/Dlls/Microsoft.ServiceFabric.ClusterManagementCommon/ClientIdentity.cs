// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.ComponentModel.DataAnnotations;

    [Serializable]
    public class ClientIdentity
    {
        public ClientIdentity()
        {
            this.IsAdmin = false;
        }

        public bool IsAdmin { get; set; }

        [Required(AllowEmptyStrings = false)]
        public string Identity { get; set; }

        public override string ToString()
        {
            return this.Identity;
        }
    }
}