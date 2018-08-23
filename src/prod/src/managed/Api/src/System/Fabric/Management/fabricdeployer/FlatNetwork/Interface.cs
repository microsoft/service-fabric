// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Xml.Serialization;

    [XmlTypeAttribute(AnonymousType = true)]
    public class Interface
    {
        private IPSubnet[] ipSubnet;

        private string isPrimary;

        private string macAddress;

        [XmlElementAttribute("IPSubnet", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public IPSubnet[] IPSubnet
        {
            get
            {
                return this.ipSubnet;
            }
            set
            {
                this.ipSubnet = value;
            }
        }

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
        public string MacAddress
        {
            get
            {
                return this.macAddress;
            }
            set
            {
                this.macAddress = value;
            }
        }
    }
}