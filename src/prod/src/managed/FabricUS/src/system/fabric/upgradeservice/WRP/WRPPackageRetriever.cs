// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Fabric.Common;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using WRP.Model;

    public interface IWrpPackageRetriever
    {
        Task<UpgradeServicePollResponse> GetWrpResponseAsync(
            UpgradeServicePollRequest pollRequest,
            CancellationToken token);
    }

    internal sealed class WrpPackageRetriever : WrpGatewayClient, IWrpPackageRetriever
    {
        private readonly TraceType traceType = new TraceType("WrpPackageRetriever");

        public WrpPackageRetriever(Uri baseUrl, string clusterId, IConfigStore configStore, string configSectionName)
            : base(baseUrl, clusterId, configStore, configSectionName)
        {
        }

        protected override TraceType TraceType
        {
            get { return this.traceType; }
        }        

        public async Task<UpgradeServicePollResponse> GetWrpResponseAsync(UpgradeServicePollRequest request, CancellationToken token)
        {
            Trace.WriteNoise(TraceType, "GetWrpServiceApplicationResponseAsync begin");
            

            var response = await this.GetWrpServiceResponseAsync<UpgradeServicePollRequest, UpgradeServicePollResponse>(
                this.BaseUrl,
                HttpMethod.Post,
                request,
                token);

            Trace.WriteNoise(TraceType, "GetWrpServiceApplicationResponseAsync end");
            return response;

        }
    }
}