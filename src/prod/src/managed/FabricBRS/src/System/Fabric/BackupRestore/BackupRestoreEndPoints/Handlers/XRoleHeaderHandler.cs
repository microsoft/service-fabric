// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Handlers
{
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.BackupRestoreEndPoints;
    using System.Fabric.Interop;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common;

    public class XRoleHeaderHandler : DelegatingHandler
    {
        private const string TraceType = "XRoleHeaderHandler";

        private CredentialType serverAuthCredentialType;

        internal XRoleHeaderHandler(CredentialType serverAuthCredentialType)
        {
            this.serverAuthCredentialType = serverAuthCredentialType;
        }

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
        {
            IEnumerable<string> xRoleHeaderValues = null;
            
            if (request.Headers.TryGetValues(BackupRestoreEndPointsConstants.FabricXRoleHeaderKey, out xRoleHeaderValues)
                )
            {
                foreach (var xRoleHeaderValue in xRoleHeaderValues)
                {
                    FabricXRole fabricXRole  = FabricXRole.Invalid;
                    if (Enum.TryParse(xRoleHeaderValue, out fabricXRole))
                    {
                        switch (fabricXRole)
                        {
                            case FabricXRole.Admin:
                            case FabricXRole.All:
                                return await base.SendAsync(request, cancellationToken);

                            case FabricXRole.User:
                                if (request.Method == HttpMethod.Get)
                                {
                                    return await base.SendAsync(request, cancellationToken);
                                }
                                break;
                            case FabricXRole.None:
                                if (serverAuthCredentialType == CredentialType.None)
                                {
                                    return await base.SendAsync(request, cancellationToken);
                                }
                                break;
                        }
                    }
                    
                }
            }
            FabricError fabricError = new FabricError()
            {
                Message = "Access Denied.",
                Code = NativeTypes.FABRIC_ERROR_CODE.E_ACCESSDENIED
            };
            BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Access Denied for message {0}", request.ToString());
            return new FabricErrorError(fabricError).ToHttpResponseMessage();
        }
    }
}