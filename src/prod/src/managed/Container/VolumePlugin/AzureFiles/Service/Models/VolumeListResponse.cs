//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin.Models
{
    using System.Collections.Generic;

    public class VolumeListResponse : Response
    {
        public VolumeListResponse(string err = "")
            : base(err)
        {
            this.Volumes = new List<VolumeMountDescription>();
        }

        public IList<VolumeMountDescription> Volumes;
    }
}
