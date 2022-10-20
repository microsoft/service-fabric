// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Host
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.IO.Pipes;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    public class AdHocHost
    {
        public static bool isWaitForDebuggerEnabled;
        private static string outputFile;
        private static List<Type> serviceTypes = new List<Type>();
        private static bool hasFabricDied = false;
        private static bool isActivationContextAvailable = false;
        private static bool recreateRuntimeOnCrash = false;
        private static string pipeServerAddress;
        private static ManualResetEvent shouldQuitNowEvent = new ManualResetEvent(false);

        public static int Run(string[] args)
        {
            AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);

            AdHocHost.Log("Starting");
            AdHocHost.ParseArguments(args);

            AdHocHost.Log("Querying activation context");
            try
            {
                FabricRuntime.GetActivationContext();
                AdHocHost.isActivationContextAvailable = true;
                AdHocHost.Log("ActivationContext get success");
            }
            catch (InvalidOperationException)
            {
                AdHocHost.Log("ActivationContext get failed. InvalidOp");
            }

            FabricRuntime runtime = null;
            Action exitedHandler = () =>
            {
                AdHocHost.Log("Fabric Exited");
                AdHocHost.hasFabricDied = true;

                // try creating the fabric runtime on a background thread
                // this should not av
                if (AdHocHost.recreateRuntimeOnCrash)
                {
                    Task.Factory.StartNew(() =>
                    {
                        while (true)
                        {
                            try
                            {
                                AdHocHost.Log("Creating again");
                                runtime = FabricRuntime.Create();
                                AdHocHost.Log("Recreated");
                                break;
                            }
                            catch (AccessViolationException)
                            {
                                AdHocHost.Log("AV");
                                throw;
                            }
                            catch (Exception ex)
                            {
                                AdHocHost.Log("Error {0} when re-creating", ex);
                            }
                        }
                    });
                }
            };

            AdHocHost.Log("Creating FabricRuntime");
            var runtimeTask = FabricRuntime.CreateAsync(exitedHandler, TimeSpan.FromSeconds(5), CancellationToken.None);

            runtimeTask.Wait();

            runtime = runtimeTask.Result;


            AdHocHost.Log("Registering factories");
            foreach (var item in AdHocHost.serviceTypes)
            {
                AdHocHost.Log("Registering: {0}", item.Name);

                var factory = new MyServiceFactory { ServiceImplementationType = item };

                Task registerTask = null;
                if (factory.IsStateful)
                {
                    registerTask = runtime.RegisterStatefulServiceFactoryAsync(item.Name, factory, TimeSpan.FromSeconds(5), CancellationToken.None);
                }
                else
                {
                    registerTask = runtime.RegisterStatelessServiceFactoryAsync(item.Name, factory, TimeSpan.FromSeconds(5), CancellationToken.None);
                }

                registerTask.Wait();
            }

            AdHocHost.Log("Connecting to pipe");
            var pipeWrapper = new PipeWrapper(AdHocHost.pipeServerAddress);

            AdHocHost.Log("Waiting for quit...");
            shouldQuitNowEvent.WaitOne();

            return 0;
        }

        static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            AdHocHost.Log("Unhandled exception: {0}", e.ExceptionObject);
        }

        private static void ParseArguments(string[] args)
        {
            AdHocHost.pipeServerAddress = args[0];
            AdHocHost.outputFile = args[1];

            for (int i = 2; i < args.Length; i++)
            {
                if (args[i] == "-debug")
                {
                    AdHocHost.isWaitForDebuggerEnabled = true;
                }
                else if (args[i] == "-recreateOnCrash")
                {
                    AdHocHost.recreateRuntimeOnCrash = true;
                }
                else
                {
                    AdHocHost.serviceTypes.Add(Type.GetType(args[i]));
                }
            }
        }

        public static void Log(string format, params object[] args)
        {
            string message = string.Format(format, args);

            DateTime time = DateTime.Now;
            string text = string.Format("{0}({1,2:00.##}:{2,2:00.##}:{3,2:00.##}.{4,3:000.##}): {5}", string.Empty, time.Hour, time.Minute, time.Second, time.Millisecond, message);

            if (!string.IsNullOrEmpty(AdHocHost.outputFile))
            {
                File.AppendAllLines(AdHocHost.outputFile, new string[] { text });
            }

            Console.WriteLine(text);
        }

        private class PipeWrapper
        {
            private readonly string pipeServerAddress;

            public PipeWrapper(string pipeServerAddress)
            {
                this.pipeServerAddress = pipeServerAddress;
                var t = new Thread(this.PipeFunc);
                t.Start();
            }

            private enum HostRequest : byte
            {
                Stop = 0,
                Query = 1,
            }

            [Flags]
            private enum HostResponse : byte
            {
                Success = 0,
                Error = 1,
                FabricHasExited = 2,
                ActivationContextIsAvailable = 4,
            }

            private void PipeFunc(object o)
            {
                NamedPipeClientStream pipe = new NamedPipeClientStream(this.pipeServerAddress);
                pipe.Connect();

                while (true)
                {
                    var pipeRequest = (HostRequest)pipe.ReadByte();

                    HostResponse resp = HostResponse.Success;

                    if (AdHocHost.hasFabricDied)
                    {
                        resp |= HostResponse.FabricHasExited;
                    }

                    if (AdHocHost.isActivationContextAvailable)
                    {
                        resp |= HostResponse.ActivationContextIsAvailable;
                    }

                    pipe.WriteByte((byte)resp);

                    if (pipeRequest == HostRequest.Stop)
                    {
                        pipe.Close();
                        AdHocHost.shouldQuitNowEvent.Set();
                        return;
                    }
                }
            }
        }

        class MyServiceFactory : IStatelessServiceFactory, IStatefulServiceFactory
        {
            public Type ServiceImplementationType { get; set; }

            public bool IsStateful
            {
                get
                {
                    return typeof(IStatefulServiceReplica).IsAssignableFrom(this.ServiceImplementationType);
                }
            }

            public IStatefulServiceReplica CreateReplica(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
            {
                return Activator.CreateInstance(this.ServiceImplementationType) as IStatefulServiceReplica;
            }

            public IStatelessServiceInstance CreateInstance(string serviceType, Uri serviceName, byte[] initializationData, Guid partitionId, long instanceId)
            {
                return Activator.CreateInstance(this.ServiceImplementationType) as IStatelessServiceInstance;
            }
        } 
    }
}