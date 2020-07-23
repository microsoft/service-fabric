// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Security;
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricEncryptText")]
    public sealed class InvokeEncryptText : CommonCmdletBase
    {
        internal const string CertStoreParameterSet = "CertStore";
        internal const string CertFileParameterSet = "CertFile";

        public InvokeEncryptText()
        {
            this.StoreLocation = StoreLocation.LocalMachine;
            this.StoreName = System.Fabric.X509Credentials.StoreNameDefault; 
        }

        [Parameter(Mandatory = true, Position = 0, HelpMessage = "Text to encrypt")]
        public string Text
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, HelpMessage = "Cryptographic object identifier of encryption algorithm to use, for example, \"2.16.840.1.101.3.4.1.42\" is AES-256-CBC")]
        public string AlgorithmOid
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = CertStoreParameterSet, HelpMessage = "Load encryption certificate from certificate store")]
        public SwitchParameter CertStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = CertFileParameterSet, HelpMessage = "Load encryption certificate from a file, supported format: DER encoded binary X509 (.CER)")]
        public SwitchParameter CertFile
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = CertStoreParameterSet, HelpMessage = "Thumbprint of encryption certificate")]
        public string CertThumbprint
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = CertStoreParameterSet, HelpMessage = "Certificate store name, default to \"My\" on Windows")]
        [ValidateNotNullOrEmpty]
        public string StoreName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = CertStoreParameterSet, HelpMessage = "Certificate store location, default to LocalMachine")]
        public StoreLocation StoreLocation
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = CertFileParameterSet, HelpMessage = "Path of encryption certificate file")]
        public string Path
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                if (this.CertStore)
                {
                    this.WriteObject(EncryptionUtility.EncryptText(
                                         this.Text, this.CertThumbprint, this.StoreName, this.StoreLocation, this.AlgorithmOid));
                }
                else if (this.CertFile)
                {
                    this.WriteObject(EncryptionUtility.EncryptTextByCertFile(this.Text, this.Path, this.AlgorithmOid));
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.InvokeEncryptTextErrorId,
                    null);
            }
        }
    }
}