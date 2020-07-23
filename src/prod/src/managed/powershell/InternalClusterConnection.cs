// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric;
    using System.Fabric.Common; 
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Repair;
    using System.Fabric.Result;
    using System.Fabric.Security;
    using System.Fabric.Strings;
    using System.Fabric.Testability.Scenario; 
    using System.Globalization;
    using System.Numerics;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class InternalClusterConnection
    {
        private IClusterConnection connection;

        internal InternalClusterConnection(
            IClusterConnection connection)
        {
            this.connection = connection;
        }

        internal Task AddUnreliableTransportAsync(
            string source, string behaviorName, UnreliableTransportBehavior behavior, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.connection.FabricClient.TestManager.AddUnreliableTransportAsync(
                       source, behaviorName, behavior, timeout, cancellationToken);
        }

        internal Task RemoveUnreliableTransportAsync(
            string source, string behaviorName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.connection.FabricClient.TestManager.RemoveUnreliableTransportAsync(
                       source, behaviorName, timeout, cancellationToken);
        }

        internal Task<IList<string>> GetTransportBehavioursAsync(
            string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.connection.FabricClient.TestManager.GetTransportBehavioursAsync(
                       nodeName, timeout, cancellationToken);
        }

        internal Task AddUnreliableLeaseAsync(
            string source, string behaviorString, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.connection.FabricClient.TestManager.AddUnreliableLeaseBehaviorAsync(
                       source, behaviorString, alias, timeout, cancellationToken);
        }

        internal Task RemoveUnreliableLeaseAsync(
            string source, string alias, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.connection.FabricClient.TestManager.RemoveUnreliableLeaseBehaviorAsync(
                       source, alias, timeout, cancellationToken);
        }
    }
}