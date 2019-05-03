// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers
{
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints;
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Net.Http;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Web.Http;

    public class BackupMappingController : BaseController
    {
        private readonly StatefulService statefulService;

        public BackupMappingController(ControllerSetting controllerSetting)
        {
            this.statefulService = controllerSetting.StatefulService;
        }

        [Route("EnableBackup")]
        [HttpPost]
        public async Task<HttpResponseMessage> EnableBackupProtection([FromBody] BackupMapping backupMapping, [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await
                this.RunAsync(new EnableBackupMappingOperation(fabricRequestHeader, backupMapping, apiVersion, this.statefulService), timeout );
        }

        [Route("GetBackupConfigurationInfo")]
        [HttpGet]
        public async Task<BackupConfigurationInfo> GetBackupProtection([FromUri(Name = "api-version")] string apiVersion = "0.0", 
                [FromUri] int timeout = 60, [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();

            return
                await
                    this.RunAsync(new GetBackupMappingOperation(fabricRequestHeader,  apiVersion, this.statefulService, continuationToken, maxResults), timeout);
        }


        [Route("DisableBackup")]
        [HttpPost]
        public async Task<HttpResponseMessage> DisableBackupProtection(CleanBackupView cleanBackupView, [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            bool cleanBackups = false;
            if(cleanBackupView != null && cleanBackupView.CleanBackup)
            {
                cleanBackups = true;
            }
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new DisableBackupMappingOperation(fabricRequestHeader, cleanBackups, apiVersion, this.statefulService),timeout);
        }
    }
}