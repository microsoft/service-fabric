// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Chaos.DataStructures;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;


    public class StartChaosRequest : FabricRequest
    {
        public StartChaosRequest(IFabricClient fabricClient, ChaosParameters chaosTestScenarioParameters, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ChaosParameters = chaosTestScenarioParameters;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
        }

        public ChaosParameters ChaosParameters { get; private set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "StartChaosAsync with Parameters={0}, timeout={1}", this.ChaosParameters, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.StartChaosAsync(this.ChaosParameters, this.Timeout, cancellationToken).ConfigureAwait(false);
        }
    }
}

#pragma warning restore 1591