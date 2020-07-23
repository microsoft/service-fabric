// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Handlers
{
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.BackupRestore;
    using System.Fabric.Interop;
    using System.Net.Http;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common;
    using System.Collections.Generic;

    public class ClientCertAuthorizationHandler : DelegatingHandler
    {
        private const string TraceType = "ClientCertAuthorizationHandler";

        private SecuritySetting fabricBrSecuritySetting;

        internal ClientCertAuthorizationHandler(SecuritySetting fabricBrSecuritySetting)
        {
            this.fabricBrSecuritySetting = fabricBrSecuritySetting;
        }

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,
            CancellationToken cancellationToken)
        {
            X509Certificate2 x509Certificate2 = request.GetClientCertificate();

            if ((x509Certificate2!=null) && (x509Certificate2.IsValid()) && 
                ((this.fabricBrSecuritySetting.ServerAuthX509FindType == X509FindType.FindByThumbprint) && this.ValidateRequestCertificateThumbprint(x509Certificate2) ||
                (this.fabricBrSecuritySetting.ServerAuthX509FindType == X509FindType.FindBySubjectName) && this.ValidateRequestCertificateByCommonName(x509Certificate2))
                )
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

        private bool ValidateRequestCertificateThumbprint(X509Certificate2 x509Certificate2)
        {
            if ((x509Certificate2?.Thumbprint != null) && 
                (this.fabricBrSecuritySetting.AllowedClientCertThumbprints.Contains(x509Certificate2.Thumbprint.ToUpper())))
            {
                return true;
            }

            return false;
        }

        private bool ValidateRequestCertificateByCommonName(X509Certificate2 x509Certificate2)
        {
            if (x509Certificate2 != null && this.fabricBrSecuritySetting.AllowedClientCertCommonNames != null && this.fabricBrSecuritySetting.AllowedClientCertCommonNames.Count > 0)
            {
                foreach (KeyValuePair<string, HashSet<string>> keyValPair in this.fabricBrSecuritySetting.AllowedClientCertCommonNames)
                {
                    if(x509Certificate2.MatchCommonName(keyValPair.Key) && x509Certificate2.ValidateChainAndIssuerThumbprint(keyValPair.Value))
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }
}