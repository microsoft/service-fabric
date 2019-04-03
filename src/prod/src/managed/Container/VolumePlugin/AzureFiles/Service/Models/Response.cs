//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace AzureFilesVolumePlugin.Models
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
