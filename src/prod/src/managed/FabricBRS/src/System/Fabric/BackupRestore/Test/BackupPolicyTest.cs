// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.BackupRestoreEndPoints;
using System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers;
using System.Fabric.BackupRestore.BackupRestoreTypes;
using System.Fabric.BackupRestore.Test.Mock;
using System.Threading;
using Microsoft.ServiceFabric.Data.Collections;
using Microsoft.ServiceFabric.Data;
using Microsoft.ServiceFabric.Services.Runtime;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace System.Fabric.BackupRestore.Test
{
    using Collections.Generic;
    using Threading.Tasks;
    using Moq;

    [TestClass]
    public class BackupPolicyTest
    {
        [TestMethod]
        public void  CreateBackupPolicyTest()
        {
            StatefulServiceContext statefulServiceContext = new StatefulServiceContext();
            var mockStatefulService = new Mock<StatefulService>();
            mockStatefulService.SetupGet<IReliableStateManager>((mockStatefulService1) =>
             mockStatefulService1.StateManager
            ).Returns(new MockIReliableStateManager());
            BackupPolicy backupPolicy = new BackupPolicy {BackupPolicyName =  "SampleBackupPolicy1"};
            ControllerSetting controllerSetting = new ControllerSetting(mockStatefulService.Object,new CancellationToken());
            BackupPolicyController backupPolicyController = new BackupPolicyController(controllerSetting);
            backupPolicyController.AddBackupPolicy(backupPolicy).Wait();
            BackupPolicy queryBackupPolicy= backupPolicyController.GetBackupPolicyByName(backupPolicy.BackupPolicyName).Result;
            if (queryBackupPolicy.Equals(backupPolicy))
            {
                Assert.Equals(queryBackupPolicy, backupPolicy);
            }
        }

    }
}