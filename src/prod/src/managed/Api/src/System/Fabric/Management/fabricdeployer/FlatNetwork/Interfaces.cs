// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Xml.Serialization;

    [XmlTypeAttribute(AnonymousType = true)]
    [XmlRootAttribute(Namespace = "", IsNullable = false)]
    public class Interfaces
    {
        private Interface[] interfaceCollection;

        [XmlElementAttribute("Interface", Form = System.Xml.Schema.XmlSchemaForm.Unqualified)]
        public Interface[] Items
        {
            get
            {
                return this.interfaceCollection;
            }
            set
            {
                this.interfaceCollection = value;
            }
        }
    }
}