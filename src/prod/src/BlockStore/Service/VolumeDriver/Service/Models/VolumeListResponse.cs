// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin.Models
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
