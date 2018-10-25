// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System.Collections.Generic;

    public interface IFolderSink
    {
        // Method invoked to inform the consumer about the locations of a set of folders
        // containing data that is of interest to the consumer
        void RegisterFolders(IEnumerable<string> folderNames);
    }
}