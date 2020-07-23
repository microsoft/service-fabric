// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.IO;

    public class MonitoringAgentServiceEvent
    {
        private string filePath;
        private string partitionId;
        private string replicaId;

        // Instance constructor is private to enforce singleton semantics
        public MonitoringAgentServiceEvent(string workDirectory, Guid partitionId, long replicaId)
        {
            if (string.IsNullOrWhiteSpace(workDirectory))
            {
                throw new ArgumentException("WorkDirectory cannot be null.");
            }

            this.filePath = Path.Combine(workDirectory, "MonitoringAgentServiceEvent.txt");
            this.partitionId = partitionId.ToString();
            this.replicaId = replicaId.ToString();

            // clean up the existing file if it exists.
            if (File.Exists(this.filePath))
            {
                File.Delete(this.filePath);
            }
        }

        public void Message(string message, params object[] args)
        {
            this.Log("Message", message, args);
        }

        public void Error(string message, params object[] args)
        {
            this.Log("Error", message, args);
        }

        private void Log(string logLevel, string message, params object[] args)
        {
            var formattedMsg = string.Format(message, args);
            var finalMessage = string.Format(
                "{0}|{1}|{2}|{3}|{4}{5}", 
                logLevel, 
                DateTime.UtcNow.ToString("F"),
                this.partitionId,
                this.replicaId,
                formattedMsg,
                Environment.NewLine);

            File.AppendAllText(this.filePath, finalMessage);
        }
    }
}