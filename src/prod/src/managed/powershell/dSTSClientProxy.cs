// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
    {
        using System;
        using System.Diagnostics.CodeAnalysis;
        using System.Fabric.dSTSClient;

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Reviewed. Suppression is OK here for dSTSClientProxy.")]
        [Serializable]
            public class dSTSClientProxy
            {
                private dSTSClient client;

                public dSTSClientProxy(
                    string metadataEndpoint,
                    string[] serverCommonName,
                    string[] serverThumbprints,
                    bool interactive,
                    string cloudServiceName,
                    string[] cloudServiceDNSNames)
                {
                    this.client = dSTSClient.CreatedSTSClient(
                                      metadataEndpoint,
                                      serverCommonName,
                                      serverThumbprints,
                                      cloudServiceName,
                                      cloudServiceDNSNames,
                                      interactive);
                }

                public string GetSecurityToken()
                {
                    return this.client.GetSecurityToken();
                }
            }
        }
