// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers
{
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Net;
    using System.Net.Http;
    using System.Threading.Tasks;
    using System.Web.Http;
    using System.Fabric.BackupRestoreEndPoints;
    using System.Threading;

    public class RestoreController : BaseController
    {
        private readonly StatefulService statefulService;

        public RestoreController(ControllerSetting controllerSetting)
        {
            this.statefulService = controllerSetting.StatefulService;
        }

        [Route("Restore")]
        [HttpPost]
        public async Task<HttpResponseMessage> RestorePartition([FromBody] RestoreRequest restoreDetails,[FromUri]int restoreTimeout = 10, [FromUri(Name = "api-version")] string apiVersion = "0.0",   [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await
                this.RunAsync(new RestorePartitionRequestOperation( fabricRequestHeader, restoreDetails ,  apiVersion, restoreTimeout, this.statefulService), timeout );
        }

        [Route("GetRestoreProgress")]
        [HttpGet]
        public async Task<RestoreProgress> GetRestoreProgress([FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new RestorePartitionStatusOperation(fabricRequestHeader, apiVersion, this.statefulService),timeout);
        }
    }
}