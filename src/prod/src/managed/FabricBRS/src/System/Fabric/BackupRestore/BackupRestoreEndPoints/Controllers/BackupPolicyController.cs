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
    using System.Net.Http;
    using System.Threading.Tasks;
    using System.Web.Http;

    [RoutePrefix("BackupPolicies")]
    public class BackupPolicyController : BaseController
    {
        private readonly StatefulService  statefulService;

        public BackupPolicyController(ControllerSetting controllerSetting)
        {
            this.statefulService = controllerSetting.StatefulService;
        }

        [Route("$/Create")]
        [HttpPost]
        public async Task<HttpResponseMessage> AddBackupPolicy([FromBody]BackupPolicy backupPolicy , [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60 )
        {
            return await this.RunAsync(
                    new AddBackupPolicyOperation(backupPolicy, apiVersion, this.statefulService), timeout);
        }

        [Route("")]
        [HttpGet]
        public async Task<PagedBackupPolicyDescriptionList> GetAllBackupPolicy([FromUri(Name = "api-version")] string apiVersion ="0.0" , [FromUri] int timeout = 60,
            [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            return await this.RunAsync(new GetAllBackupPolicyOperation(apiVersion, this.statefulService, continuationToken, maxResults), timeout);
        }

        [Route("{backupPolicyName}/$/GetBackupEnabledEntities")]
        [HttpGet]
        public async Task<PagedBackupEntityList> GetAllEnablementByPolicyName(string backupPolicyName, [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60,
            [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            return await this.RunAsync( new GetBackupPolicyProtectedItemsOperation(backupPolicyName, apiVersion, this.statefulService, continuationToken, maxResults) , timeout);
        }

        [Route("{backupPolicyName}")]
        [HttpGet]
        public async Task<BackupPolicy> GetBackupPolicyByName(string backupPolicyName , [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            return await this.RunAsync(
                new GetBackupPolicyByNameOperation(backupPolicyName, apiVersion, this.statefulService),timeout);
        }

        [Route("{backupPolicyName}/$/Delete")]
        [HttpPost]
        public async Task<HttpResponseMessage> DeleteBackupPolicyByName(string backupPolicyName , [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            return await this.RunAsync(new DeleteBackupPolicyOperation(backupPolicyName, apiVersion, this.statefulService), timeout);
        }

        [Route("{backupPolicyName}/$/Update")]
        [HttpPost]
        public async Task<HttpResponseMessage> UpdateBackupPolicyByName([FromBody] BackupPolicy backupPolicy , string backupPolicyName , [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60)
        {
            return await this.RunAsync(new UpdateBackupPolicyOperation(backupPolicyName, backupPolicy, apiVersion, this.statefulService),timeout);
        }
    }
}