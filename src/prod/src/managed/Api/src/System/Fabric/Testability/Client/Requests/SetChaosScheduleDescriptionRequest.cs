// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class SetChaosScheduleRequest : FabricRequest
    {
        public SetChaosScheduleRequest(IFabricClient fabricClient, string jsonString, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ScheduleJson = jsonString;
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_KEY_NOT_FOUND);
        }

        public string ScheduleJson
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "SetChaosScheduleRequest with timeout={0}", this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.SetChaosScheduleAsync(this.ScheduleJson, this.Timeout, cancellationToken).ConfigureAwait(false);
        }
    }
}

#pragma warning restore 1591