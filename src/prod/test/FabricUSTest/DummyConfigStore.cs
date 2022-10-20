// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricUS.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal class DummyConfigStore : IConfigStore
    {
        bool IConfigStore.IgnoreUpdateFailures
        {
            get
            {
                throw new NotImplementedException();
            }

            set
            {
                throw new NotImplementedException();
            }
        }

        ICollection<string> IConfigStore.GetKeys(string sectionName, string partialKeyName)
        {
            throw new NotImplementedException();
        }

        ICollection<string> IConfigStore.GetSections(string partialSectionName)
        {
            throw new NotImplementedException();
        }

        string IConfigStore.ReadString(string sectionName, string keyName, out bool isEncrypted)
        {
            throw new NotImplementedException();
        }

        string IConfigStore.ReadUnencryptedString(string sectionName, string keyName)
        {
            return string.Empty;
        }
    }
}