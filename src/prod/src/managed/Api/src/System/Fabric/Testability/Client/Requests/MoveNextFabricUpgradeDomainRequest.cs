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
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class MoveNextFabricUpgradeDomainRequest : MoveNextDomainRequest<FabricUpgradeProgress>
    {
        public MoveNextFabricUpgradeDomainRequest(IFabricClient fabricClient, FabricUpgradeProgress currentProgress, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(currentProgress, "currentProgress");

            this.CurrentProgress = currentProgress;
            this.ConfigureErrorCodes();
        }

        public override string ToString()
        {
            string progress = this.CurrentProgress == null ? "null" : this.CurrentProgress.ToString();
            return string.Format(CultureInfo.InvariantCulture, "Move next upgrade domain for fabric with progress {0} and timeout {1}", progress, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.MoveNextFabricUpgradeDomainAsync(this.CurrentProgress, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.S_OK);
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_FABRIC_NOT_UPGRADING);

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT);
        }
    }
}


#pragma warning restore 1591