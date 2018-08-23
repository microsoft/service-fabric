// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    
    internal interface IConfigStore
    {
        ICollection<string> GetSections(string partialSectionName);

        ICollection<string> GetKeys(string sectionName, string partialKeyName);

        string ReadUnencryptedString(string sectionName, string keyName);

        string ReadString(string sectionName, string keyName, out bool isEncrypted);

        bool IgnoreUpdateFailures { get; set; } 
    }
}