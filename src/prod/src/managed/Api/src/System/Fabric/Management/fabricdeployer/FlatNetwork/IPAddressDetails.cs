// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Xml.Serialization;

    [XmlTypeAttribute(AnonymousType = true)]
    public class IPAddressDetails
    {
        private string isPrimary;

        private string address;

        [XmlAttributeAttribute()]
        public string IsPrimary
        {
            get
            {
                return this.isPrimary;
            }
            set
            {
                this.isPrimary = value;
            }
        }

        [XmlAttributeAttribute()]
        public string Address
        {
            get
            {
                return this.address;
            }
            set
            {
                this.address = value;
            }
        }
    }
}