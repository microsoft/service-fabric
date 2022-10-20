// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using System.ComponentModel;
    using System.Globalization;
    using System.Text;
    using Newtonsoft.Json;

    [JsonObject(IsReference = false)]
    public class RuntimePackageDetails
    {
        public string Version
        {
            get;
            set;
        }
        
        public DateTime? SupportExpiryDate
        {
            get;
            set;
        }       

        public string TargetPackageLocation
        {
            get;
            set;
        }

        [DefaultValue(false)]
        public bool IsGoalPackage
        {
            get;
            set;
        }

        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendFormat(CultureInfo.CurrentCulture, "Version: {0}; ", this.Version ?? string.Empty);
            sb.AppendFormat(CultureInfo.CurrentCulture, "SupportExpiryDate: {0}; ", this.SupportExpiryDate);
            sb.AppendFormat(CultureInfo.CurrentCulture, "TargetPackageLocation: {0}; ", this.TargetPackageLocation ?? string.Empty);
            return sb.ToString();
        }

        internal static RuntimePackageDetails CreateFromPackageDetails(PackageDetails packageDetails)
        {
            return new RuntimePackageDetails
            {
                Version = packageDetails.Version,
                SupportExpiryDate = packageDetails.SupportExpiryDate,
                TargetPackageLocation = packageDetails.TargetPackageLocation,
                IsGoalPackage = packageDetails.IsGoalPackage
            };
        }
    }
}