// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    internal class ApplicationInstanceContext
    {
        public ApplicationInstanceContext(
            ApplicationInstanceType appInstance,
            ApplicationPackageType appPackage,
            IEnumerable<ServicePackageType> servicePackages,
            IDictionary<string, string> mergedApplicationParameters)
        {
            this.ApplicationInstance = appInstance;
            this.ApplicationPackage = appPackage;
            this.ServicePackages = servicePackages;
            this.MergedApplicationParameters = mergedApplicationParameters;
        }

        public ApplicationInstanceType ApplicationInstance { get; private set; }
        public ApplicationPackageType ApplicationPackage { get; private set; }
        public IEnumerable<ServicePackageType> ServicePackages { get; private set; }
        public IDictionary<string, string> MergedApplicationParameters { get; private set; }
    }
}