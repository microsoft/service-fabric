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
    internal class PackageDetails
    {
        public string Version
        {
            get;
            set;
        }

        [DefaultValue("0.0.0.0")]
        public string MinVersion
        {
            get;
            set;
        }

        public DateTime? SupportExpiryDate
        {
            get;
            set;
        }

        [DefaultValue(false)]
        public bool IsUpgradeDisabled
        {
            get;
            set;
        }

        [DefaultValue(false)]
        public bool IsDowngradeDisabled
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
            sb.AppendFormat(CultureInfo.CurrentCulture, "MinVersion: {0}; ", this.MinVersion ?? string.Empty);
            sb.AppendFormat(CultureInfo.CurrentCulture, "SupportExpiryDate: {0}; ", this.SupportExpiryDate);
            sb.AppendFormat(CultureInfo.CurrentCulture, "IsUpgradeDisabled: {0}; ", this.IsUpgradeDisabled);
            sb.AppendFormat(CultureInfo.CurrentCulture, "IsDowngradeDisabled: {0}; ", this.IsDowngradeDisabled);
            sb.AppendFormat(CultureInfo.CurrentCulture, "TargetPackageLocation: {0}; ", this.TargetPackageLocation ?? string.Empty);
            sb.AppendFormat(CultureInfo.CurrentCulture, "IsGoalPackage: {0}; ", this.IsGoalPackage);

            return sb.ToString();
        }
    }
}