// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace MS.Test.ImageStoreService.Controllers
{
    using System;
    using System.Fabric;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Mvc;

    [Route("api/[controller]")]
    public class WorkloadController : Controller
    {
        private static FabricClient FabricClient = new FabricClient();
        private readonly string ApplicationPackageFolder = "DummyApp";
        private readonly string ImageStoreConnectionString = "fabric:ImageStore";

        [HttpPost("/upload")]
        public void Upload()
        {
            var currentDirectory = Directory.GetCurrentDirectory();
            var appDirectory = Path.Combine(currentDirectory, this.ApplicationPackageFolder);
            var timeout = TimeSpan.FromMinutes(4);
            FabricClient.ApplicationManager.CopyApplicationPackage(this.ImageStoreConnectionString, appDirectory, this.ApplicationPackageFolder, timeout);
        }

        [HttpPost("/delete")]
        public void Delete()
        {
            FabricClient.ApplicationManager.RemoveApplicationPackage(this.ImageStoreConnectionString, this.ApplicationPackageFolder);
        }
    }
}
