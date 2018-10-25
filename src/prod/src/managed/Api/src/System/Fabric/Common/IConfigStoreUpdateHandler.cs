// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    internal interface IConfigStoreUpdateHandler
    {
        bool OnUpdate(string sectionName, string keyName);
        bool CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted);
    }
}