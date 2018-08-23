// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ServiceModel
{
    using System.Globalization;

    internal class FabricVersion
    {
        public static FabricVersion Invalid = new FabricVersion("0.0.0.0", string.Empty);

        public FabricVersion(string codeVersion, string configVersion)
        {
            this.CodeVersion  = codeVersion;
            this.ConfigVersion = configVersion;
        }

        public string CodeVersion
        {
            get;
            private set;
        }

        public string ConfigVersion
        {
            get;
            private set;
        }       

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}:{1}", this.CodeVersion, this.ConfigVersion);
        }

        public static bool TryParse(string fabricVersionString, out FabricVersion fabricVersion)
        {
            fabricVersion = null;

            if (string.IsNullOrEmpty(fabricVersionString))
            {
                return false;
            }

            int index = fabricVersionString.IndexOf(':');
            if (index == -1)
            {
                return false;
            }

            string codeVersion = fabricVersionString.Substring(0, index);
            string configVersion = fabricVersionString.Substring(index + 1);

            fabricVersion = new FabricVersion(codeVersion, configVersion);

            return true;
        }

        public override bool Equals(object obj)
        {
            FabricVersion other = obj as FabricVersion;

            return (other != null)
                && (string.Equals(this.CodeVersion, other.CodeVersion, System.StringComparison.OrdinalIgnoreCase))
                && (string.Equals(this.ConfigVersion, this.ConfigVersion, System.StringComparison.OrdinalIgnoreCase));
        }

        public override int GetHashCode()
        {
            return this.CodeVersion.GetHashCode() + this.ConfigVersion.GetHashCode();
        }
    }
}