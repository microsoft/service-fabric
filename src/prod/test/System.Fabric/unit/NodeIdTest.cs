// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Numerics;
    using System.Fabric.Description;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Diagnostics;

    [TestClass]
    public class NodeIdTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("pasharma")]
        public void NodeId_ParseNoHigh()
        {
            Random random = new Random(1);
            NodeId nodeId = new NodeId(new BigInteger(0), new BigInteger(random.Next(int.MaxValue)));
            NodeId parsed;
            LogHelper.Log("Node Id: {0}", nodeId.ToString());
            Assert.IsTrue(NodeId.TryParse(nodeId.ToString(), out parsed), "Parse failed");
            Assert.AreEqual(nodeId, parsed, "Parsed nodeID not same as original one");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("pasharma")]
        public void NodeId_Parse()
        {
            Random random = new Random(1);
            NodeId nodeId = new NodeId(new BigInteger(random.Next(int.MaxValue)), new BigInteger(random.Next(int.MaxValue)));
            LogHelper.Log("Node Id: {0}", nodeId.ToString());
            NodeId parsed;
            Assert.IsTrue(NodeId.TryParse(nodeId.ToString(), out parsed), "Parse failed");
            Assert.AreEqual(nodeId, parsed, "Parsed nodeID not same as original one");
        }
    }
}