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

    public class BackupRestorePointsController : BaseController
    {
        private readonly StatefulService statefulService;

        // RestorePoints are referred as BackupList in Swagger-spec.
        public BackupRestorePointsController(ControllerSetting controllerSetting)
        {
            this.statefulService = controllerSetting.StatefulService;
        }

        [Route("GetBackups")]
        [HttpGet]
        public async Task<PagedBackupInfoList> GetRestorePoints([FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60,
            [FromUri] bool latest = false, [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new GetRestorePointsOperation(fabricRequestHeader, latest, apiVersion, this.statefulService, null, null,continuationToken, maxResults),timeout);
            
        }

        [Route("GetBackups")]
        [HttpGet]
        public async Task<PagedBackupInfoList> GetRestorePointsWithinDates([FromUri] DateTime startDateTimeFilter, [FromUri] DateTime endDateTimeFilter,
            [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60, [FromUri] bool latest = false,
            [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new GetRestorePointsOperation(fabricRequestHeader, latest, apiVersion, this.statefulService, startDateTimeFilter,
                endDateTimeFilter, continuationToken, maxResults), timeout);

        }
        
        [Route("GetBackups")]
        [HttpGet]
        public async Task<PagedBackupInfoList> GetRestorePointsWithStartDate([FromUri(Name = "StartDateTimeFilter")] DateTime startDateTimeFilter,
            [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60, [FromUri] bool latest = false,
            [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new GetRestorePointsOperation(fabricRequestHeader, latest, apiVersion, this.statefulService, startDateTimeFilter,
                null, continuationToken, maxResults), timeout);
        }

        [Route("GetBackups")]
        [HttpGet]
        public async Task<PagedBackupInfoList> GetRestorePointsWithEndDate([FromUri(Name = "EndDateTimeFilter")] DateTime endDateTimeFilter,
            [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60, [FromUri] bool latest = false,
            [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            string fabricRequestHeader = this.GetFabricRequestFromRequstHeader();
            return await this.RunAsync(new GetRestorePointsOperation(fabricRequestHeader, latest, apiVersion, this.statefulService, null,
                endDateTimeFilter, continuationToken, maxResults), timeout);
        }

        [Route("$/GetBackups")]
        [HttpPost]
        public async Task<PagedBackupInfoList> GetRestorePointsWithBackupStorage([FromBody] BackupByStorageQueryDescription backupByStorageQueryDescription,
            [FromUri(Name = "api-version")] string apiVersion = "0.0", [FromUri] int timeout = 60,
            [FromUri(Name = "ContinuationToken")]string continuationToken = null, [FromUri(Name = "MaxResults")]int maxResults = 0)
        {
            return await this.RunAsync(new GetRestorePointsWithBackupStorageOperation(backupByStorageQueryDescription, apiVersion, this.statefulService, continuationToken, maxResults), timeout);
        }
    }
}