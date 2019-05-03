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

    public class MoveNextApplicationUpgradeDomainRequest : MoveNextDomainRequest<ApplicationUpgradeProgress>
    {
        public MoveNextApplicationUpgradeDomainRequest(
            IFabricClient fabricClient, 
            ApplicationUpgradeProgress currentProgress, 
            TimeSpan timeout, 
            uint extraSucceedErrorCode)
            : this(fabricClient, currentProgress, timeout)
        {
            this.SucceedErrorCodes.Add(extraSucceedErrorCode);
        }

        public MoveNextApplicationUpgradeDomainRequest(IFabricClient fabricClient, ApplicationUpgradeProgress currentProgress, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(currentProgress, "currentProgress");

            this.CurrentProgress = currentProgress;
            this.ConfigureErrorCodes();
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Move next upgrade domain for application {0} with timeout {1}", this.CurrentProgress.ApplicationName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.MoveNextApplicationUpgradeDomainAsync(this.CurrentProgress, this.Timeout, cancellationToken);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.S_OK);
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_UPGRADING);

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.E_ABORT);
        }
    }
}


#pragma warning restore 1591