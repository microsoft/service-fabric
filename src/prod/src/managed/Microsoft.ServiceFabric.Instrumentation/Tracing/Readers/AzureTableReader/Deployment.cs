// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Globalization;

    /// <summary>
    /// Represents each Deployment
    /// </summary>
    internal class Deployment
    {
        internal Deployment(string deploymentId, DateTimeOffset startTime)
        {
            if (string.IsNullOrWhiteSpace(deploymentId))
            {
                throw new ArgumentNullException("deploymentId");
            }

            this.DeploymentId = deploymentId;
            this.StartTime = startTime;
        }

        public string DeploymentId { get; }

        public DateTimeOffset StartTime { get; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "ID: {0}, StartTime: {1}", this.DeploymentId, this.StartTime);
        }
    }
}