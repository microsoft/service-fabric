// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Handlers
{
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;

    public class XContentTypeHandler : DelegatingHandler
    {
        private const string TraceType = "XContentTypeHandler";

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            HttpResponseMessage response = await base.SendAsync(request, cancellationToken);
            response.Headers.Add("X-Content-Type-Options", "nosniff");
            return response;
        }
    }
}