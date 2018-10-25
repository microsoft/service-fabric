// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    internal interface IConfigReader
    {
        bool IsReadingFromApplicationManifest { get; }

        T GetUnencryptedConfigValue<T>(string sectionName, string valueName, T defaultValue);

        string ReadString(string sectionName, string valueName, out bool isEncrypted);

        string GetApplicationLogFolder();

#if !DotNetCoreClr
        string GetApplicationEtlFilePath();
#endif
        string GetApplicationType();

        IEnumerable<Guid> GetApplicationEtwProviderGuids();

        Dictionary<string, List<ServiceEtwManifestInfo>> GetApplicationEtwManifests();

        IEnumerable<string> GetConsumersForProducer(string producerInstance);
    }
}