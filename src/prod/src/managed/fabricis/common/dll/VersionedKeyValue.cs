// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    internal sealed class VersionedKeyValue : IVersionedKeyValue
    {
        public VersionedKeyValue(string key, string versionKey, string value, Int64 version)
        {
            this.Key = key.Validate("key");
            this.VersionKey = versionKey.Validate("versionKey");
            this.Value = value;
            this.Version = version;
        }

        public string Key { get; private set; }

        public string VersionKey { get; private set; }

        public string Value { get; private set; }

        /// <summary>
        /// The version associated with the value
        /// </summary>
        /// <remarks>Using <see cref="Int64"/> for compatibility with Naming Store.</remarks>
        public Int64 Version { get; private set; }

        public override string ToString()
        {
            return "Key: {0}, VersionKey: {1}, Value: {2}, Version: {3}".ToString(Key, VersionKey, Value, Version);
        }
    }
}