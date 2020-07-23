//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin.Models
{
    using System.Collections.Generic;

    public class VolumeCreateRequest
    {
        public string Name;

        public Dictionary<string, string> Opts;
    }
}
