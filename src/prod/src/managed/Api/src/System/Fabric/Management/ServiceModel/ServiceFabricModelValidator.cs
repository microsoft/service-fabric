// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ServiceModel
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;

    public partial class ApplicationManifestType : IXmlValidator
    {
        public void Validate()
        {
            if (this.ServiceManifestImport == null || this.ServiceManifestImport.Length == 0 || this.ServiceManifestImport[0] == null)
                throw new InvalidOperationException("The element 'ApplicationManifest' in namespace 'http://schemas.microsoft.com/2011/01/fabric' has incomplete content.List of possible elements expected: 'ServiceManifestImport' in namespace 'http://schemas.microsoft.com/2011/01/fabric'. ");
        }
    }

    public partial class ServiceManifestType : IXmlValidator
    {
        public void Validate()
        {
            if (this.ServiceTypes == null || this.ServiceTypes.Length == 0 || this.ServiceTypes[0] == null)
                throw new InvalidOperationException("The element 'ServiceManifest' in namespace 'http://schemas.microsoft.com/2011/01/fabric' has incomplete content.List of possible elements expected: 'ServiceTypes' in namespace 'http://schemas.microsoft.com/2011/01/fabric'. ");
            else if (this.CodePackage == null || this.CodePackage.Length == 0 || this.CodePackage[0] == null)
                throw new InvalidOperationException("The element 'ServiceManifest' in namespace 'http://schemas.microsoft.com/2011/01/fabric' has incomplete content.List of possible elements expected: 'CodePackage' in namespace 'http://schemas.microsoft.com/2011/01/fabric'. ");
        }
    }
}