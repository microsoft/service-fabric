// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Interop;
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricDecryptText")]
    public sealed class InvokeDecryptText : CommonCmdletBase
    {
        public InvokeDecryptText()
        {
            this.StoreLocation = StoreLocation.LocalMachine;
        }

        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true, HelpMessage = "Ciphertext in base64 format")]
        public string CipherText
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, HelpMessage = "Certificate store location, default to LocalMachine")]
        public StoreLocation StoreLocation
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                this.WriteObject(this.DecryptText(this.CipherText, this.StoreLocation));
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.InvokeDecryptTextErrorId,
                    null);
            }
        }
    }
}