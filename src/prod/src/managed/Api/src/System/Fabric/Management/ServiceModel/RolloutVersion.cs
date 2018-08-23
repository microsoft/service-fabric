// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ServiceModel
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;

    internal class RolloutVersion
    {
        private RolloutVersion(long majorVersion, long minorVersion)
        {
            if (majorVersion < 1)
            {
                throw new ArgumentException("Value has to be a positive integer.", "majorVersion");
            }

            if (minorVersion < 0)
            {
                throw new ArgumentException("Value has to be a non-negative integer.", "minorVersion");
            }

            this.MajorVersion = majorVersion;
            this.MinorVersion = minorVersion;
        }

        public long MajorVersion
        {
            get;
            private set;
        }

        public long MinorVersion
        {
            get;
            private set;
        }

        public static RolloutVersion CreateRolloutVersion(long majorVersion = 1)
        {
            return new RolloutVersion(majorVersion, 0);
        }

        public static RolloutVersion CreateRolloutVersion(string rolloutVersion)
        {
            if (rolloutVersion == null)
            {
                throw new ArgumentNullException("rolloutVersion");
            }

            string[] versions = rolloutVersion.Split('.');

            long majorVersion, minorVersion;

            if (versions.Length == 2 && long.TryParse(versions[0], out majorVersion) && long.TryParse(versions[1], out minorVersion))
            {
                return new RolloutVersion(majorVersion, minorVersion);
            }
            else
            {
                throw new ArgumentException("RolloutVersion string {0} is not in the correct format.", rolloutVersion);
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public RolloutVersion NextMajorRolloutVersion()
        {
            return new RolloutVersion(this.MajorVersion + 1, 0);
        }

        public RolloutVersion NextMinorRolloutVersion()
        {
            return new RolloutVersion(this.MajorVersion, this.MinorVersion + 1);
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "{0}.{1}", this.MajorVersion, this.MinorVersion);
        }

        public override bool Equals(object obj)
        {
            RolloutVersion other = obj as RolloutVersion;

            return (other != null)
                && (other.MajorVersion == this.MajorVersion)
                && (other.MinorVersion == this.MinorVersion);
        }

        public override int GetHashCode()
        {
            return (int)((this.MajorVersion * 1000) + this.MinorVersion);
        }
    }
}