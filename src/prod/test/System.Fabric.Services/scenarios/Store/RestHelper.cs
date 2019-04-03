// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Services.Scenarios.Store.Test
{
    using Fabric.Test;
    using Fabric.Test.Common.FabricUtility;
    using IO;
    using Net;

    /// <summary>
    /// Internal helper class containing extension methods for simple REST based operations.
    /// </summary>
    internal static class RestHelper
    {
        public static string SendGetRequest(this FabricDeployment deployment)
        {
            ResolvedServicePartition rsp = null;
            for (int i = 0; i < 5; i++)
            {
                LogHelper.Log("Resolving try {0}", i);
                rsp = deployment.Resolve(Constants.ApplicationName, Constants.ServiceTypeName, null, rsp);
                var ep = rsp.GetEndpoint().Address;
                LogHelper.Log("Endpoint {0}", ep);

                try
                {
                    return SendRequest(ep, "GET");
                }
                catch (Exception ex)
                {
                    LogHelper.Log("Exception: {0}", ex);
                }
            }

            throw new InvalidOperationException();
        }        

        public static string SendPostRequest(this FabricDeployment deployment, string requestMessage = "")
        {
            ResolvedServicePartition rsp = null;
            for (int i = 0; i < 5; i++)
            {
                LogHelper.Log("Resolving try {0}", i);
                rsp = deployment.Resolve(Constants.ApplicationName, Constants.ServiceTypeName, null, rsp);
                var ep = rsp.GetEndpoint().Address;
                LogHelper.Log("Endpoint {0}", ep);

                try
                {
                    return SendRequest(ep, "POST", requestMessage);
                }
                catch (Exception ex)
                {
                    LogHelper.Log("Exception: {0}", ex);
                }
            }

            throw new InvalidOperationException("Failed after count");
        }

        private static string SendRequest(string address, string method, string requestMessage = "")
        {
            LogHelper.Log("Sending {0} message: {1}", method, string.IsNullOrWhiteSpace(requestMessage) ? "<empty>" : requestMessage);

            var req = (HttpWebRequest)WebRequest.Create(address);
            req.Method = method;

            if (req.Method == "POST")
            {
                using (var sw = new StreamWriter(req.GetRequestStream()))
                {
                    sw.Write(requestMessage);
                }
            }

            using (var resp = (HttpWebResponse)req.GetResponse())
            {
                using (var sr = new StreamReader(resp.GetResponseStream()))
                {
                    string response = sr.ReadToEnd();
                    LogHelper.Log("Getting response '{0}' for {1} message: {2}", response, method, string.IsNullOrWhiteSpace(requestMessage) ? "<empty>" : requestMessage);
                    return response;
                }
            }
        }        
    }
}