// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Runtime.Serialization.Json;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ServiceModel;
using System.ServiceModel.Channels;
using System.Fabric;
using System.Fabric.Common;
using System.Fabric.Description;
using Microsoft.WindowsAzure.Security.Authentication;
using Microsoft.WindowsAzure.Security.Authentication.Contracts;
using System.IdentityModel.Tokens;
using System.Net;
using System.Net.Security;
using System.Fabric.Strings;
using System.Fabric.Common.Tracing;

namespace System.Fabric.dSTSClient
{
    public class dSTSClientImpl
    {
        private const string TokenID = @"http://schemas.microsoft.com/dsts/saml2-bearer";
        private const string metadataSuffix = @"/$/GetDstsMetadata";
        private const string metadataVersion = @"?api-version=1.0";
        private static readonly string TraceType = "DSTSClient";

        private readonly Uri metadataUrl;
        private AuthenticationClient authenticationClient;
        private string[] serverCommonNames;
        private string[] serverThumbprints;
        private bool interactive;
        private RemoteCertificateValidationCallback certValidationCallback;
        private string cloudServiceName;
        private string[] cloudServiceDnsNames; 

        internal static FabricEvents.ExtensionsEvents TraceSource = FabricEvents.ExtensionsEvents.GetEventSource(FabricEvents.Tasks.DSTSClient);

        private dSTSClientImpl(
            string metadataEndpoint,
            string[] serverCommonNames,
            string[] serverThumbprints,
            bool interactive)
            : this(
                metadataEndpoint,
                serverCommonNames,
                serverThumbprints,
                interactive,
                null,
                null)
        {
        }

        private dSTSClientImpl(
            string metadataEndpoint,
            string[] serverCommonNames,
            string[] serverThumbprints,
            bool interactive,
            string cloudServiceName,
            string[] cloudServiceDnsNames)
        {
            this.metadataUrl = new Uri("https://" + metadataEndpoint.TrimEnd('/') + metadataSuffix + metadataVersion);
            this.serverCommonNames = serverCommonNames;
            this.serverThumbprints = serverThumbprints;
            this.interactive = interactive;
            this.cloudServiceName = cloudServiceName;
            this.cloudServiceDnsNames = cloudServiceDnsNames;

            TraceSource.WriteInfo(
                TraceType,
                "Creating DSTSClient with metadataurl: {0}, ServerCommonNames: {1}, ServerThumbprints: {2}, CloudServiceName: {3}, CloudServiceDNSNames: {4}, Interactive: {5}",
                metadataUrl,
                serverCommonNames != null ? string.Join(",", serverCommonNames) : "",
                serverThumbprints != null ? string.Join(",", serverThumbprints) : "",
                cloudServiceName != null ? cloudServiceName : "",
                cloudServiceDnsNames != null? string.Join(",", cloudServiceDnsNames) : "",
                interactive.ToString());
        }

        ~dSTSClientImpl()
        {
            if (certValidationCallback != null)
            {
                ServicePointManager.ServerCertificateValidationCallback -= certValidationCallback;
            }
        }

        private bool Initialize()
        {
            IClientCredential credential;
            credential = ClientCredential.CreateWindowsCredential();
            AuthenticationClientConfiguration config = new AuthenticationClientConfiguration();
            config.IsInteractive = interactive;
            authenticationClient = new AuthenticationClient(credential, null, config);
            certValidationCallback = new RemoteCertificateValidationCallback(new CertificateHandler(serverCommonNames, serverThumbprints).ValidateCertificate);

            //
            // The ssl server certificate used in httpgateway might not have a subject name that
            // matches the uri. So we need to add custom handler to validate the server subject name with
            // the ServerCommonName passed in.
            //
            ServicePointManager.ServerCertificateValidationCallback += certValidationCallback;

            return true;
        }

        public string GetSecurityToken()
        {
            WebSecurityToken webSecurityToken = new WebSecurityToken(
                TokenID,
                GetSecurityTokenInternal(),
                new GenericXmlSecurityTokenSerializer());

            return webSecurityToken.ToAuthorizationHeader();
        }

        internal SecurityToken GetSecurityTokenInternal()
        {
            TokenServiceMetadata gatewayMetadata = GetMetadata();
            AuthenticationMetadata dSTSMetadata = new AuthenticationMetadata(TVSSerializerUtility.Deserialize(gatewayMetadata.Metadata));

            if (cloudServiceName != null && cloudServiceName != gatewayMetadata.ServiceName)
            {
                string warning = string.Format(StringResources.Error_dSTSMismatchInMetadata, "CloudServiceName", cloudServiceName, gatewayMetadata.ServiceName);

                TraceSource.WriteWarning(
                    TraceType,
                    warning);
                throw new FabricException(warning);
            }

            if (cloudServiceDnsNames != null &&
                cloudServiceDnsNames.FirstOrDefault(name => name == gatewayMetadata.ServiceDnsName) == null)
            {
                string warning = string.Format(StringResources.Error_dSTSMismatchInMetadata, "CloudServiceDNSNames", string.Join(",", cloudServiceDnsNames), gatewayMetadata.ServiceDnsName);

                TraceSource.WriteWarning(
                    TraceType,
                    warning);
                throw new FabricException(warning);
            }

            SecurityTokenIssuanceResponse rstr;
            try
            {
                rstr = authenticationClient.GetSecurityToken(
                    gatewayMetadata.ServiceName,
                    gatewayMetadata.ServiceDnsName,
                    dSTSMetadata);
            }
            catch (SecurityTokenIssuanceException e)
            {
                TraceSource.WriteWarning(
                    TraceType,
                    "GetSecurityToken failed with exception: {0}",
                    e.Message);

                throw new FabricException(e.Message);
            }

            return rstr.SecurityToken;
        }

        private TokenServiceMetadata GetMetadata()
        {
            TokenServiceMetadata metadata = null;
            try
            {
                metadata = TryGetMetadata();
            }
            catch (Exception e)
            {
                TraceSource.WriteWarning(
                    TraceType,
                    "GetMetadata failed with exception: {0}",
                    e.Message);

                throw new FabricTransientException(e.Message, e, FabricErrorCode.CommunicationError);
            }

            TraceSource.WriteNoise(
                TraceType,
                "DSTS Service metadata: ServiceName:{0}, ServiceDNSName:{1}, Metadata:{2}",
                metadata.ServiceName,
                metadata.ServiceDnsName,
                metadata.Metadata);

            return metadata;
        }

        private TokenServiceMetadata TryGetMetadata()
        {
            HttpWebResponse response = null;
            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(metadataUrl);
            req.Method = "GET";

            response = (HttpWebResponse)req.GetResponse();

            DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(TokenServiceMetadata));
            return (TokenServiceMetadata)deserializer.ReadObject(response.GetResponseStream());
        }

        public static dSTSClientImpl CreatedSTSClient(
            string metadataUrl,
            string[] serverCommonName,
            string[] serverThumbprints,
            bool interactive = false,
            string cloudServiceName = null,
            string[] cloudServiceDNSNames = null)
        {
            dSTSClientImpl client = new dSTSClientImpl(
                metadataUrl, 
                serverCommonName, 
                serverThumbprints,
                interactive,
                cloudServiceName,
                cloudServiceDNSNames);

            if (client.Initialize())
            {
                return client;
            }

            return null;
        }
    }
}
