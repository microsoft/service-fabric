// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Description;
    using System.Threading;
    using System.Threading.Tasks;

    public sealed class TokenValidationService : IStatelessServiceInstance, ITokenValidationService
    {
        private static readonly string TraceType = "TokenValidationService";

        private readonly FabricTokenValidationServiceAgent serviceAgent;
        private readonly ITokenValidationProvider validationProvider;
        private Guid partitionId;
        private long instanceId;

        internal TokenValidationService(
            FabricTokenValidationServiceAgent serviceAgent, 
            ITokenValidationProvider validationProvider)
        {
            this.serviceAgent = serviceAgent;
            this.validationProvider = validationProvider;
        }

        public void Initialize(StatelessServiceInitializationParameters initializationParameters)
        {
            this.partitionId = initializationParameters.PartitionId;
            this.instanceId = initializationParameters.InstanceId;
        }

        public Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken)
        {
            TokenValidationServiceFactory.TraceSource.WriteInfo(
                TokenValidationService.TraceType,
                "Opening TokenValidationService: partition={0} instance={1}",
                this.partitionId,
                this.instanceId);

            this.serviceAgent.RegisterTokenValidationService(this.partitionId, this.instanceId, this);

            var tcs = new TaskCompletionSource<string>();
            tcs.SetResult("Completed");
            return tcs.Task;
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            TokenValidationServiceFactory.TraceSource.WriteInfo(
                TokenValidationService.TraceType,
                "Closing TokenValidationService: partition={0} instance={1}",
                this.partitionId,
                this.instanceId);

            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        public void Abort()
        {
        }

        #region ITokenValidationService

        Task<ClaimDescriptionList> ITokenValidationService.ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            TokenValidationServiceFactory.TraceSource.WriteNoise(
                TokenValidationService.TraceType, 
                "ValidateToken: partition={0} instance={1}",
                this.partitionId,
                this.instanceId);

            return this.validationProvider.ValidateTokenAsync(authToken, timeout, cancellationToken);
        }

        public TokenServiceMetadata GetTokenServiceMetadata()
        {
            return this.validationProvider.GetTokenServiceMetadata();
        }

        #endregion
    }
}