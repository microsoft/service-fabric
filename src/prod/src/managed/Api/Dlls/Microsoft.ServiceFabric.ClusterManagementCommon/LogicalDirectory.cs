// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    [Serializable]
    public class LogicalDirectory
    {
        public string LogicalDirectoryName { get; set; }

        public string MappedTo { get; set; }

        public LogicalDirectoryTypeContext Context { get; set; }

        public LogicalDirectoryType ToLogicalDirectoryType()
        {
            LogicalDirectoryType logicalDirectory = new LogicalDirectoryType()
            {
                LogicalDirectoryName = this.LogicalDirectoryName,
                MappedTo = this.MappedTo,
                Context = this.Context
            };

            return logicalDirectory;
        }
    }
}