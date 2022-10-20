// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Handlers
{
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Net.Http;
    using System.Security.Principal;
    using System.Threading;
    using System.Threading.Tasks;

    public class ClientIdentityAuthorizationHandler : DelegatingHandler
    {
        private const string TraceType = "ClientIdentityAuthorizationHandler";

        private SecuritySetting fabricBrSecuritySetting;

        internal ClientIdentityAuthorizationHandler(SecuritySetting fabricBrSecuritySetting)
        {
            this.fabricBrSecuritySetting = fabricBrSecuritySetting;
        }

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,
            CancellationToken cancellationToken)
        {
            if (this.ValidateRequestIdentity(request))
            {
                return await base.SendAsync(request, cancellationToken);
            }
            
            FabricError fabricError = new FabricError()
            {
                Message = "Access Denied.",
                Code = NativeTypes.FABRIC_ERROR_CODE.E_ACCESSDENIED
            };
            BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Access Denied for message {0}", request.ToString());
            return new FabricErrorError(fabricError).ToHttpResponseMessage();
        }

        private bool ValidateRequestIdentity(HttpRequestMessage request)
        {
            WindowsPrincipal user = request.GetOwinContext().Authentication.User as WindowsPrincipal;

            if (user != null)
            {
                // Validation should pass if the request's user identity is same as Current identity.
                if (user.Identity.Equals(WindowsPrincipal.Current.Identity))
                {
                    return true;
                }

                foreach (string clusterIdentity in fabricBrSecuritySetting.AllowedClientIdentities)
                {
                    if (user.IsInRole(clusterIdentity))
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }
}