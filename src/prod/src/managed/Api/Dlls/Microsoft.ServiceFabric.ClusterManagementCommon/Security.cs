// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using Newtonsoft.Json;

    public enum CredentialType
    {
        None,
        X509,
        Windows
    }

    /// <summary>
    /// The input portion from CSM: i.e. what can be specified in a put request
    /// for a cluster resource.
    /// </summary>
    [JsonObject(IsReference = true)]
    [Serializable]
    public class Security
    {
        public CredentialType ClusterCredentialType { get; set; }

        public CredentialType ServerCredentialType { get; set; }

        public X509 CertificateInformation { get; set; }

        public Windows WindowsIdentities { get; set; }

        public AzureActiveDirectory AzureActiveDirectory { get; set; }

        public Security Clone()
        {
            IFormatter formatter = new BinaryFormatter();
            Stream stream = new MemoryStream();
            using (stream)
            {
                formatter.Serialize(stream, this);
                stream.Seek(0, SeekOrigin.Begin);
                return (Security)formatter.Deserialize(stream);
            }
        }
    }
}