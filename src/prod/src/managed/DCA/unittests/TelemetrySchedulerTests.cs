// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Fabric.Dca;
    using Moq;

    [TestClass]
    public class TelemetrySchedulerTests
    {
        private readonly static string TelemetryPushTime = "11:11:11";
        private readonly static string TelemetryDailyPushes = TelemetryScheduler.DefaultTelemetryDailyPushes.ToString();
        private static Mock<IConfigReader> MockConfigReader;
        private static bool TestOnlyPushTimeParameterSet = false;

        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
            TelemetrySchedulerTests.MockConfigReader = new Mock<IConfigReader>(MockBehavior.Strict);

            // defining mock configReader return values
            TelemetrySchedulerTests.MockConfigReader.Setup(cr => cr.GetUnencryptedConfigValue(It.IsAny<string>(), It.IsAny<string>(), It.IsAny<string>()))
            .Returns<string, string, string>
            (
            (section, parameter, dvalue) =>
            {
                if (parameter == ConfigReader.TestOnlyTelemetryDailyPushesParamName)
                {
                    return TelemetrySchedulerTests.TelemetryDailyPushes;
                }
                if (parameter == ConfigReader.TestOnlyTelemetryPushTimeParamName)
                {
                    // testing case where parameter is not set
                    if (TelemetrySchedulerTests.TestOnlyPushTimeParameterSet)
                    {
                        return TelemetrySchedulerTests.TelemetryPushTime;
                    }
                    else
                    {
                        return string.Empty;
                    }
                }

                return "NotDefinedParameter";
            }
            );
        }

        [TestMethod]
        public void TestSetupNotTestOnlyPushTimeSet()
        {
            TelemetrySchedulerTests.TestOnlyPushTimeParameterSet = false;
            // push time is not defined in this scenario
            TelemetryScheduler telScheduler = new TelemetryScheduler(MockConfigReader.Object);

            // overriding the current time for a time before the push time
            telScheduler.SetupFromSettingsFile(MockConfigReader.Object, new TimeSpan(10, 12, 11));

            // testing if SendInterval, AlignedPushTime, and SendDelay are being properly computed
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryDailyPushes, telScheduler.DailyPushes);
            Assert.AreEqual(TimeSpan.FromHours(24.0 / TelemetryScheduler.DefaultTelemetryDailyPushes), telScheduler.SendInterval);
            Assert.AreEqual(new TimeSpan(2, 42, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);
        }

        // Push time for this test is NOT set
        [TestMethod]
        public void TestSendDelayLastBatchPushTimeNotTestOnlyParameterNotSet()
        {
            TelemetrySchedulerTests.TestOnlyPushTimeParameterSet = false;
            // push time is not defined in this scenario
            TelemetryScheduler telScheduler = new TelemetryScheduler(MockConfigReader.Object);

            // Testing scenario where current time is BEFORE push time (which should be ignored in this test method)
            // overriding the current time for a time BEFORE the push time (which is 11:11:11)
            telScheduler.SetupFromSettingsFile(MockConfigReader.Object, new TimeSpan(10, 12, 11));

            telScheduler.UpdateScheduler(false, false, new TimeSpan(17, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(2, 42, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);

            // not persisted and already sent, this case should not exist  but if it happens it should start from scratch as if it was the first time
            telScheduler.UpdateScheduler(false, true, new TimeSpan(17, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(2, 42, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);

            // persisted but not sent yet just ignore and recompute push delay
            telScheduler.UpdateScheduler(true, false, new TimeSpan(17, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(2, 42, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);

            // persisted and already sent so set delay to align push with previous push time (1st case pushtime - currenttime < sendinterval)
            telScheduler.UpdateScheduler(true, true, new TimeSpan(17, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(17, 5, 3), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(6, 52, 52), telScheduler.SendDelay);

            // persisted and already sent so set delay to align push with previous push time (2st case pushtime - currenttime == sendinterval)
            telScheduler.UpdateScheduler(true, true, new TimeSpan(18, 12, 11), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(18, 12, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 0, 0), telScheduler.SendDelay);

            // persisted and already sent so set delay to align push with previous push time (3st case pushtime - currenttime > sendinterval)
            telScheduler.UpdateScheduler(true, true, new TimeSpan(19, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(19, 5, 3), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 52, 52), telScheduler.SendDelay);

            // Testing scenario where current time is AFTER push time (which should be ignored in this test method)
            // overriding the current time for a time AFTER the push time (which is 11:11:11)
            telScheduler.SetupFromSettingsFile(MockConfigReader.Object, new TimeSpan(14, 10, 11));

            // not persisted and not sent, should recompute the push time and delay
            telScheduler.UpdateScheduler(false, false, new TimeSpan(17, 5, 3), new TimeSpan(14, 10, 11));
            Assert.AreEqual(new TimeSpan(6, 40, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);

            // not persisted and already sent, this case should not exist  but if it happens it should start from scratch as if it was the first time
            telScheduler.UpdateScheduler(false, true, new TimeSpan(17, 5, 3), new TimeSpan(14, 10, 11));
            Assert.AreEqual(new TimeSpan(6, 40, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);

            // persisted but not sent yet just ignore and recompute push delay
            telScheduler.UpdateScheduler(true, false, new TimeSpan(17, 5, 3), new TimeSpan(14, 10, 11));
            Assert.AreEqual(new TimeSpan(6, 40, 11), telScheduler.PushTime);
            Assert.AreEqual(TelemetryScheduler.DefaultTelemetryInitialSendDelayInHours, telScheduler.SendDelay.TotalHours);

            // persisted and already sent so set delay to align push with previous push time (1st case pushtime - currenttime < sendinterval)
            telScheduler.UpdateScheduler(true, true, new TimeSpan(1, 5, 3), new TimeSpan(23, 10, 11));
            Assert.AreEqual(new TimeSpan(1, 5, 3), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(1, 54, 52), telScheduler.SendDelay);

            // persisted and already sent so set delay to align push with previous push time (2st case pushtime - currenttime == sendinterval)
            telScheduler.UpdateScheduler(true, true, new TimeSpan(7, 10, 11), new TimeSpan(23, 10, 11));
            Assert.AreEqual(new TimeSpan(7, 10, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 0, 0), telScheduler.SendDelay);

            // persisted and already sent so set delay to align push with previous push time (3st case pushtime - currenttime > sendinterval)
            telScheduler.UpdateScheduler(true, true, new TimeSpan(23, 5, 3), new TimeSpan(23, 10, 11));
            Assert.AreEqual(new TimeSpan(23, 5, 3), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(7, 54, 52), telScheduler.SendDelay);
        }

        // Push time for this test IS set
        [TestMethod]
        public void TestSendDelayLastBatchPushTimeNotTestOnlyParameterSet()
        {
            TelemetrySchedulerTests.TestOnlyPushTimeParameterSet = true;
            // push time is defined in this scenario and is set to (11:11:11)
            TelemetryScheduler telScheduler = new TelemetryScheduler(MockConfigReader.Object);

            // Testing scenario where current time is BEFORE push time (which should be ignored in this test method)
            // overriding the current time for a time BEFORE the push time (which is 11:11:11)
            telScheduler.SetupFromSettingsFile(MockConfigReader.Object, new TimeSpan(10, 12, 11));

            telScheduler.UpdateScheduler(false, false, new TimeSpan(17, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 59, 0), telScheduler.SendDelay);

            // not persisted and already sent, this case should not exist  but if it happens it should start from scratch as if it was the first time
            telScheduler.UpdateScheduler(false, true, new TimeSpan(17, 5, 3), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 59, 0), telScheduler.SendDelay);

            // persisted but not sent yet just ignore and recompute push delay (it should use as push time the value specified at config file)
            telScheduler.UpdateScheduler(true, false, new TimeSpan(22, 12, 13), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0,59,0), telScheduler.SendDelay);

            // First considering the case where the specified push time has not changed
            telScheduler.UpdateScheduler(true, true, new TimeSpan(11, 11, 11), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 59, 0), telScheduler.SendDelay);

            // Now considering the case where the specified push time has changed
            telScheduler.UpdateScheduler(true, true, new TimeSpan(17, 11, 11), new TimeSpan(10, 12, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(0, 59, 0), telScheduler.SendDelay);

            // Testing scenario where current time is AFTER push time
            // overriding the current time for a time AFTER the push time (which is 11:11:11)
            telScheduler.SetupFromSettingsFile(MockConfigReader.Object, new TimeSpan(14, 10, 11));

            // not persisted and not sent, should recompute the push time and delay
            telScheduler.UpdateScheduler(false, false, new TimeSpan(17, 5, 3), new TimeSpan(14, 10, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(21,1,0), telScheduler.SendDelay);

            // not persisted and already sent, this case should not exist  but if it happens it should start from scratch as if it was the first time
            telScheduler.UpdateScheduler(false, true, new TimeSpan(17, 5, 3), new TimeSpan(14, 10, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(21, 1, 0), telScheduler.SendDelay);

            // persisted but not sent yet just ignore and recompute push delay
            telScheduler.UpdateScheduler(true, false, new TimeSpan(17, 5, 3), new TimeSpan(14, 10, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(21, 1, 0), telScheduler.SendDelay);

            // First considering the case where the specified push time has not changed
            telScheduler.UpdateScheduler(true, true, new TimeSpan(11, 11, 11), new TimeSpan(23, 11, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(4, 0, 0), telScheduler.SendDelay);

            // Now considering the case where the specified push time has changed
            telScheduler.UpdateScheduler(true, true, new TimeSpan(10, 11, 11), new TimeSpan(23, 11, 11));
            Assert.AreEqual(new TimeSpan(11, 11, 11), telScheduler.PushTime);
            Assert.AreEqual(new TimeSpan(12, 0, 0), telScheduler.SendDelay);
        }
    }
}