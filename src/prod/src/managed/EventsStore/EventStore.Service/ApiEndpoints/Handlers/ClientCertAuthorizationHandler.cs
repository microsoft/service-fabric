// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Handlers
{
    using System.Net.Http;
    using System.Fabric.Interop;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Collections.Generic;
    using EventStore.Service.ApiEndpoints.Server;
    using EventStore.Service.Types;
    using EventStore.Service.ApiEndpoints.Common;
    using EventStore.Service;
    using EventStore.Service.LogProvider;

    public class ClientCertAuthorizationHandler : DelegatingHandler
    {
        private SecuritySetting fabricSecuritySetting;

        internal ClientCertAuthorizationHandler(SecuritySetting fabricSecuritySetting)
        {
            this.fabricSecuritySetting = fabricSecuritySetting;
        }

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,
            CancellationToken cancellationToken)
        {
            X509Certificate2 x509Certificate2 = request.GetClientCertificate();

            if ((x509Certificate2 != null) && (x509Certificate2.IsValid()) && 
                ((this.fabricSecuritySetting.AuthX509FindType == X509FindType.FindByThumbprint) && this.ValidateRequestCertificateThumbprint(x509Certificate2) ||
                (this.fabricSecuritySetting.AuthX509FindType == X509FindType.FindBySubjectName) && this.ValidateRequestCertificateByCommonName(x509Certificate2))
                )
            {
                return await base.SendAsync(request, cancellationToken);
            }
            
            FabricError fabricError = new FabricError
            {
                Message = "Access Denied.",
                Code = NativeTypes.FABRIC_ERROR_CODE.E_ACCESSDENIED
            };
            EventStoreLogger.Logger.LogWarning("Access Denied for message {0}", request.ToString());
            return new FabricErrorError(fabricError).ToHttpResponseMessage();
        }

        private bool ValidateRequestCertificateThumbprint(X509Certificate2 x509Certificate2)
        {
            if ((x509Certificate2?.Thumbprint != null) && 
                (this.fabricSecuritySetting.AllowedClientCertThumbprints.Contains(x509Certificate2.Thumbprint.ToUpper())))
            {
                return true;
            }

            return false;
        }

        private bool ValidateRequestCertificateByCommonName(X509Certificate2 x509Certificate2)
        {
            if (x509Certificate2 != null && this.fabricSecuritySetting.AllowedClientCertCommonNames != null && this.fabricSecuritySetting.AllowedClientCertCommonNames.Count > 0)
            {
                foreach (KeyValuePair<string, HashSet<string>> keyValPair in this.fabricSecuritySetting.AllowedClientCertCommonNames)
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