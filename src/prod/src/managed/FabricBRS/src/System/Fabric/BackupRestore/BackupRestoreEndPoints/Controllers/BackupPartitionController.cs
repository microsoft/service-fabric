// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Net;
using System.Net.Http;

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers
{
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Threading.Tasks;
    using System.Web.Http;

    public class BackupPartitionController : BaseController
    {
        private readonly StatefulService statefulService;

        public BackupPartitionController(ControllerSetting controllerSetting)
        {
            this.statefulService = controllerSetting.StatefulService;
        }

        [Route("Backup")]
        [HttpPost]
        public async Task<HttpResponseMessage> BackupPartition([FromBody] BackupPartitionRequest backupStorage,[FromUri] int backupTimeout = 10 , [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new BackupPartitionRequestOperation(fabricRequestHeader, backupStorage?.BackupStorage, backupTimeout, apiVersion, this.statefulService), timeout);
        }

        [Route("GetBackupProgress")]
        [HttpGet]
        public async Task<BackupProgress> GetBackupPartitionProgress([FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new  BackupPartitionResponseOperation(fabricRequestHeader, apiVersion, this.statefulService), timeout);
        }
    }
}