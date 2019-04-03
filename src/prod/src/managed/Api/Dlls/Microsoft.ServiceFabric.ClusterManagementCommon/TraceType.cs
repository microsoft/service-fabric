// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public sealed class TraceType
    {
        public TraceType(string name)
        {
            this.Name = name;
        }

        public string Name { get; private set; }
    }
}