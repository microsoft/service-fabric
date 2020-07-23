// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace BSDockerVolumePlugin.Models
{
    public class Response
    {
        public Response(string err = "")
        {
            this.Err = err;
        }

        public string Err;
    }
}
