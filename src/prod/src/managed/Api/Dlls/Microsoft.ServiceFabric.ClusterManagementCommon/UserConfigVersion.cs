// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public class UserConfigVersion
    {
        public UserConfigVersion(string version)
        {
            this.Version = version;
        }

        public string Version
        {
            get;
            set;
        }

        public override bool Equals(object other)
        {
            var otherVersion = other as UserConfigVersion;
            if (otherVersion == null)
            {
                return false;
            }

            return this.Version.Equals(otherVersion.Version);
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.Version.GetHashCode();
            return hash;
        }

        public override string ToString()
        {
            return this.Version;
        }
    }
}