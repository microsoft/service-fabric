// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "Todo")]
    public class InvokeEncryptSecretRequest : FabricRequest
    {
        public InvokeEncryptSecretRequest(IFabricClient fabricClient, string certThumbPrint, string certStoreLocation, string text)
            : base(fabricClient, TimeSpan.MaxValue/*API does not take a timeout*/)
        {
            ThrowIf.NullOrEmpty(certThumbPrint, "certThumbPrint");
            ThrowIf.NullOrEmpty(certStoreLocation, "certStoreLocation");
            ThrowIf.NullOrEmpty(text, "text");

            this.CertThumbPrint = certThumbPrint;
            this.CertStoreLocation = certStoreLocation;
            this.Text = text;
        }

        public string CertThumbPrint
        {
            get;
            private set;
        }

        public string CertStoreLocation
        {
            get;
            private set;
        }

        public string Text
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Test InvokeEncryptSecretRequest certThumbPrint: {0} certStoreLocation: {1} text: {2} timeout: {3})", this.CertThumbPrint, this.CertStoreLocation, this.Text, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.InvokeEncryptSecretAsync(this.CertThumbPrint, this.CertStoreLocation, this.Text, cancellationToken);
        }
    }
}


#pragma warning restore 1591