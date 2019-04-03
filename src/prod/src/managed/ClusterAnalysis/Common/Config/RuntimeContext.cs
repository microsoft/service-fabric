// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Config
{
    using System.Globalization;

    public sealed class RuntimeContext
    {
        /// <summary>
        /// Gets the Fabric code path
        /// </summary>
        public string FabricCodePath { get; set; }

        /// <summary>
        /// Gets the Fabric Data Root
        /// </summary>
        public string FabricDataRoot { get; set; }

        /// <summary>
        /// Gets the Fabric Log Root
        /// </summary>
        public string FabricLogRoot { get; set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "FabricCodePath: '{0}', FabricDataRoot: '{1}', FabricLogRoot: '{2}'",
                this.FabricCodePath,
                this.FabricDataRoot,
                this.FabricLogRoot);
        }
    }
}