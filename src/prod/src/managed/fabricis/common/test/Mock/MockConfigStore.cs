// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal class MockConfigStore : IConfigStore
    {
        private readonly Dictionary<string, string> store = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        public ICollection<string> GetSections(string partialSectionName)
        {
            throw new NotImplementedException();
        }

        public ICollection<string> GetKeys(string sectionName, string partialKeyName)
        {
            throw new NotImplementedException();
        }

        public string ReadUnencryptedString(string sectionName, string keyName)
        {
            string value;
            bool success = store.TryGetValue(sectionName + "/" + keyName, out value);

            return success ? value : string.Empty;
        }

        public string ReadString(string sectionName, string keyName, out bool isEncrypted)
        {
            throw new NotImplementedException();
        }

        public bool IgnoreUpdateFailures { get; set; }

        public void AddKeyValue(string sectionName, string keyName, string value)
        {
            store.Add(sectionName + "/" + keyName, value);
        }

        public void UpdateKey(string sectionName, string keyName, string value)
        {
            store[sectionName + "/" + keyName] = value;
        }

        public string GetValue(string sectionName, string keyName)
        {
            return ReadUnencryptedString(sectionName, keyName);
        }

        public void ClearKeys()
        {
            store.Clear();
        }
    }
}