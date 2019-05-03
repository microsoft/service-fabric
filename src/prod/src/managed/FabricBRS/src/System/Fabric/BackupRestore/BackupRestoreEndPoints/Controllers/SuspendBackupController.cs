// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Net.Http;

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers
{
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations;
    using System.Threading.Tasks;
    using System.Web.Http;

    public class SuspendBackupController : BaseController
    {
        private readonly StatefulService statefulService;

        public SuspendBackupController(ControllerSetting controllerSetting)
        {
            this.statefulService = controllerSetting.StatefulService;
        }

        [Route("SuspendBackup")]
        [HttpPost]
        public async Task<HttpResponseMessage> SuspendBackupPartition([FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new SuspendBackupOperation(fabricRequestHeader, apiVersion, this.statefulService) , timeout );
        }

        [Route("ResumeBackup")]
        [HttpPost]
        public async Task<HttpResponseMessage> ResumeBackupPartition([FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new  ResumeBackupOperation(fabricRequestHeader, apiVersion, this.statefulService), timeout);
        }
    }
}