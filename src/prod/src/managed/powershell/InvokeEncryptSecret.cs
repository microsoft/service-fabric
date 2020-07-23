// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Security;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricEncryptSecret")]
    public sealed class InvokeEncryptSecret : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string CertThumbPrint
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string CertStoreLocation
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2)]
        public string Text
        {
            get;
            set;
        }

        private new int? TimeoutSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                this.WriteWarning("This command is being deprecated, please use Invoke-ServiceFabricEncryptText");
                this.WriteObject(EncryptionUtility.EncryptText(
                                     this.Text,
                                     this.CertThumbPrint,
                                     this.CertStoreLocation));
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.InvokeEncryptSecretErrorId,
                    null);
            }
        }
    }
}