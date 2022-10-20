// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricUS.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.UpgradeService;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    static class WrpStreamChannelExtension
    {
        static public async Task StartStreamChannelNoThrowAsync<T>(this WrpStreamChannel wrpStreamChannel, CancellationToken token)
        {
            try
            {
                await wrpStreamChannel.StartStreamChannel(token).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                if (e is T)
                {
                    return;
                }
                else
                {
                    throw;
                }
            }            
        }
    }
}