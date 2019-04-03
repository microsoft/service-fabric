// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot.Test
{
    using Autopilot;
    using Collections.Generic;
    using InfrastructureService.Test;
    using Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Repair;
    using Threading.Tasks;

    [TestClass]
    public class AskModeContextTest
    {
        private MockConfigStore configStore;
        private AskModeContext context;
        private IRepairManager rm = new NullRepairManager();

        [TestInitialize]
        public void TestInitialize()
        {
            configStore = new MockConfigStore();

            var configReader = new ConfigReader(new ConfigSection(new TraceType(Constants.TraceTypeName), configStore, "IS"));
            context = new AskModeContext(configReader, "fabric:/IS");
        }

        private async Task ReconcileExecute(string[] expectedActions)
        {
            var actionList = context.Reconcile();

            string[] actualActions = actionList.Select(a => a.ToString()).ToArray();

            var missing = expectedActions.Except(actualActions);
            var unexpected = actualActions.Except(expectedActions);

            Assert.AreEqual(
                0,
                missing.Count() + unexpected.Count(),
                "Expected but not returned: {{ {0} }}, returned but not expected: {{ {1} }}",
                string.Join(", ", missing),
                string.Join(", ", unexpected));

            foreach (var action in actionList)
            {
                await action.ExecuteAsync(rm);
            }

            context.UpdateRepairDelays();
        }

        class TestRepairRecord
        {
            private TestRepairRecord()
            {
            }

            public static TestRepairRecord CreatePending(
                string machineName,
                string repairType,
                TimeSpan delay,
                bool isApproved = false,
                bool isDelayModified = false)
            {
                return new TestRepairRecord()
                {
                    Record = MachineMaintenanceRecord.FromPendingRepair(
                        new MaintenanceRecordId(machineName, repairType),
                        delay),
                    IsApproved = isApproved,
                    IsDelayModified = isDelayModified,
                };
            }

            public static TestRepairRecord CreateActive(
                string machineName,
                string repairType,
                bool isApproved = false,
                bool isDelayModified = false)
            {
                return new TestRepairRecord()
                {
                    Record = MachineMaintenanceRecord.FromActiveRepair(
                        new MaintenanceRecordId(machineName, repairType)),
                    IsApproved = isApproved,
                    IsDelayModified = isDelayModified,
                };
            }

            public MachineMaintenanceRecord Record { get; set; }
            public bool IsApproved { get; set; }
            public bool IsDelayModified { get; set; }
        }

        private async Task RunCommonTestAsync(TestRepairRecord[] testRecords, IList<IRepairTask> repairTasks, string[] expectedActions)
        {
            foreach (var r in testRecords)
            {
                context.SetRepairRecord(r.Record);
            }

            context.SetRepairTasks(repairTasks);

            await ReconcileExecute(expectedActions);

            foreach (var r in testRecords)
            {
                Assert.AreEqual(r.IsApproved, r.Record.IsApproved);
                Assert.AreEqual(r.IsDelayModified, r.Record.IsDelayModified);
            }
        }

        [TestMethod]
        public async Task DMPending_RMNone()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromHours(72)),
                },
                new IRepairTask[]
                {
                },
                new string[]
                {
                    "CreateRepairTaskInPreparing:MACHINE1/SoftReboot:Restart:False",
                });
        }

        [TestMethod]
        public async Task DMPending_RMCompleted()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromHours(72)),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Completed,
                    },
                },
                new string[]
                {
                    "CreateRepairTaskInPreparing:MACHINE1/SoftReboot:Restart:False",
                });
        }

        [TestMethod]
        public async Task DMPending_RMPreparing()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromHours(72)),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Preparing,
                        PreparingTimestamp = DateTime.UtcNow.AddMinutes(-5),
                    },
                },
                new string[]
                {
                });

            Assert.AreEqual(0, this.context.OverdueRepairTaskCount);
        }

        [TestMethod]
        public async Task DMPending_RMPreparing_Overdue()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromHours(72)),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Preparing,
                        PreparingTimestamp = DateTime.UtcNow.AddMinutes(-500),
                    },
                },
                new string[]
                {
                });

            Assert.AreEqual(1, this.context.OverdueRepairTaskCount);
        }

        [TestMethod]
        public async Task DMPending_RMApproved()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending(
                        "machine1", "SoftReboot", TimeSpan.FromHours(72),
                        isApproved: true,
                        isDelayModified: true),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Approved,
                        ApprovedTimestamp = DateTime.UtcNow.AddMinutes(-10),
                    },
                },
                new string[]
                {
                    "ExecuteRepair:MACHINE1/SoftReboot:AP/MACHINE1/SoftReboot/20170401-030813",
                });
        }

        [TestMethod]
        public async Task DMPending_RMApprovedRecently()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromHours(72)),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Approved,
                        ApprovedTimestamp = DateTime.UtcNow,
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMPending_RMExecuting()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending(
                        "machine1", "SoftReboot", TimeSpan.FromHours(72),
                        isApproved: true,
                        isDelayModified: true),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                    },
                },
                new string[]
                {
                    "ExecuteRepair:MACHINE1/SoftReboot:AP/MACHINE1/SoftReboot/20170401-030813",
                });
        }

        [TestMethod]
        public async Task DMPending_RMExecuting_DoNotIncreaseDelayWhenApproved()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending(
                        "machine1", "SoftReboot", TimeSpan.FromMinutes(1),
                        isApproved: true,
                        isDelayModified: false), // because delay is already short
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                    },
                },
                new string[]
                {
                    "ExecuteRepair:MACHINE1/SoftReboot:AP/MACHINE1/SoftReboot/20170401-030813",
                });
        }

        [TestMethod]
        public async Task DMPending_RMExecuting_NegativeDMDelay()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromMinutes(-5)),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMPendingAndActive_RMExecuting()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromMinutes(-5)),
                    TestRepairRecord.CreateActive("machine1", "SoftReboot"),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMActive_RMExecuting()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreateActive("machine1", "SoftReboot"),
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMActive_RMNone_NoOp()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreateActive("machine1", "NoOp"),
                },
                new IRepairTask[]
                {
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMNone_RMExecuting_Recent()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                        ExecutingTimestamp = DateTime.UtcNow.AddMinutes(-2),
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMNone_RMExecuting_Old()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                        ExecutingTimestamp = DateTime.UtcNow.AddMinutes(-20),
                    },
                },
                new string[]
                {
                    "MoveRepairTaskToRestoring:AP/MACHINE1/SoftReboot/20170401-030813"
                });
        }

        [TestMethod]
        public async Task DMNone_RMExecuting_Old_DisallowCancel()
        {
            this.context.DoNotCancelRepairsForMachine("MACHINE1");

            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Executing,
                        ExecutingTimestamp = DateTime.UtcNow.AddMinutes(-20),
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMNone_RMRestoring()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                },
                new IRepairTask[]
                {
                    new MockRepairTask("AP/MACHINE1/SoftReboot/20170401-030813", "SoftReboot")
                    {
                        State = RepairTaskState.Restoring,
                    },
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMNone_RMNone()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                },
                new IRepairTask[]
                {
                },
                new string[]
                {
                });
        }

        [TestMethod]
        public async Task DMPendingMultiple_RMNone()
        {
            await RunCommonTestAsync(
                new TestRepairRecord[]
                {
                    TestRepairRecord.CreateActive("machine1", "SoftReboot"),
                    TestRepairRecord.CreateActive("machine2", "HardReboot"),
                    TestRepairRecord.CreateActive("machine3", "SwitchService"),
                    TestRepairRecord.CreateActive("machine4", "Reassign"),
                    TestRepairRecord.CreatePending("machine1", "SoftReboot", TimeSpan.FromHours(72)),
                    TestRepairRecord.CreatePending("machine2", "HardReboot", TimeSpan.FromHours(72)),
                    TestRepairRecord.CreatePending("machine3", "SwitchService", TimeSpan.FromHours(72)),
                    TestRepairRecord.CreatePending("machine4", "Reassign", TimeSpan.FromHours(72)),
                },
                new IRepairTask[]
                {
                },
                new string[]
                {
                    "CreateRepairTaskInPreparing:MACHINE1/SoftReboot:Restart:False",
                    "CreateRepairTaskInPreparing:MACHINE2/HardReboot:Restart:False",
                    "CreateRepairTaskInPreparing:MACHINE3/SwitchService:Restart:False",
                    "CreateRepairTaskInPreparing:MACHINE4/Reassign:RemoveData:False",
                });
        }
    }
}