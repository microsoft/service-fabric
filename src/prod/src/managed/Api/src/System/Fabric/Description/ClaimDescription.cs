// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ClaimDescription 
    {
        private string claimType;
        private string issuer;
        private string originalIssuer;
        private string subject;
        private string value;
        private string valueType;

        public ClaimDescription(string claimType, string issuer, string originalIssuer, string subject, string value, string valueType)
        {
            this.claimType = claimType;
            this.issuer = issuer;
            this.originalIssuer = originalIssuer;
            this.subject = subject;
            this.value = value;
            this.valueType = valueType;
        }

        public string ClaimType
        {
            get
            {
                return this.claimType;
            }
        }

        public string Issuer
        {
            get
            {
                return this.issuer;
            }
        }

        public string OriginalIssuer
        {
            get
            {
                return this.originalIssuer;
            }
        }

        public string Subject
        {
            get
            {
                return this.subject;
            }
        }

        public string Value
        {
            get
            {
                return this.value;
            }
        }

        public string ValueType
        {
            get
            {
                return this.valueType;
            }
        }

        internal unsafe void ToNative(PinCollection pin, out NativeTypes.FABRIC_TOKEN_CLAIM description)
        {
            description.ClaimType = pin.AddObject(this.claimType);
            description.Issuer = pin.AddObject(this.issuer);
            description.OriginalIssuer = pin.AddObject(this.originalIssuer);
            description.Subject = pin.AddObject(this.subject);
            description.Value = pin.AddObject(this.value);
            description.ValueType = pin.AddObject(this.valueType);
            description.Reserved = IntPtr.Zero;
        }
    }
}