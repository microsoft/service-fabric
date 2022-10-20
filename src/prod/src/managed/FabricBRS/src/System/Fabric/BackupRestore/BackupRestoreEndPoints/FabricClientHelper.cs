// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints
{
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers;
    using System.Web.Http;
    using Microsoft.Owin;
    using Owin;
    using Microsoft.Owin.Hosting;
    using Microsoft.ServiceFabric.Services.Runtime;

    internal static class FabricClientHelper
    {
        private static FabricClient fabricClient = new FabricClient();

        internal static async Task<bool>  ValidateApplicationOrServiceUri(string applicationOrServiceUri)
        {
            return await fabricClient.PropertyManager.NameExistsAsync(new Uri(applicationOrServiceUri));
        }
    }
}