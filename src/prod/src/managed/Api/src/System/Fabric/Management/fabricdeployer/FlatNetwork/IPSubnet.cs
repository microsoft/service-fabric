// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Xml.Serialization;
    public class IPSubnet
    {
        private IPAddressDetails[] ipAddressDetailsCollection;

        private string prefixField;

        [XmlElementAttribute("IPAddress", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public IPAddressDetails[] IPAddress
        {
            get
            {
                return this.ipAddressDetailsCollection;
            }
            set
            {
                this.ipAddressDetailsCollection = value;
            }
        }

        [XmlAttributeAttribute()]
        public string Prefix
        {
            get
            {
                return this.prefixField;
            }
            set
            {
                this.prefixField = value;
            }
        }
    }
}