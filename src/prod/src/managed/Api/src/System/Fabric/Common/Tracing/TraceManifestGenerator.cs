// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Xml;
    using System.Xml.XPath;

    internal class TraceManifestGenerator
    {
        private StringBuilder eventsSection;
        private StringBuilder templatesSection;
        private StringBuilder mapsSection;
        private StringBuilder tasksSection;
        private StringBuilder keywordsSection;
        private StringBuilder resourcesSection;

        private Dictionary<uint, Dictionary<string, uint>> stringTable;

        public TraceManifestGenerator()
        {
            this.eventsSection = new StringBuilder(10240);
            this.templatesSection = new StringBuilder(10240);
            this.mapsSection = new StringBuilder(10240);
            this.tasksSection = new StringBuilder(10240);
            this.keywordsSection = new StringBuilder(1024);
            this.resourcesSection = new StringBuilder(10240);

            this.stringTable = new Dictionary<uint, Dictionary<string, uint>>(1024);
        }

        public StringBuilder EventsSectionWriter
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.eventsSection;
            }
        }

        public StringBuilder TemplatesSectionWriter
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.templatesSection;
            }
        }

        public StringBuilder MapsSectionWriter
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.mapsSection;
            }
        }

        public StringBuilder TasksSectionWriter
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.tasksSection;
            }
        }

        public StringBuilder KeywordsSectionWriter
        {
            [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
            get
            {
                return this.keywordsSection;
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public string StringResource(string original)
        {
            uint hashValue = (uint) original.GetHashCode();
            uint differentiator;
            if (this.stringTable.ContainsKey(hashValue))
            {
                Dictionary<string, uint> sameHashStrings = this.stringTable[hashValue];
                if (sameHashStrings.ContainsKey(original))
                {
                    differentiator = sameHashStrings[original];
                }
                else
                {
                    differentiator = (uint) sameHashStrings.Keys.Count;
                    sameHashStrings[original] = differentiator;
                }
            }
            else
            {
                differentiator = 0;
                Dictionary<string, uint> sameHashStrings = new Dictionary<string, uint>();
                sameHashStrings[original] = differentiator;
                this.stringTable[hashValue] = sameHashStrings;
            }

            return string.Format(CultureInfo.InvariantCulture, "$(string.ms{0}.{1})", hashValue, differentiator);
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public void Merge(string inputFile)
        {
            // Use XmlTextReader instead of XmlReader because the latter normalizes attribute values, which results in
            // removal of some newline characters from our message strings. This is not desirable because it regresses
            // the format in which traces are displayed in TraceViewer.
            using (XmlTextReader reader = new XmlTextReader(inputFile) { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null })
            {
                XPathDocument doc = new XPathDocument(reader, XmlSpace.Preserve);
                XPathNavigator navigator = doc.CreateNavigator();

                XmlNamespaceManager manager = new XmlNamespaceManager(navigator.NameTable);
                manager.AddNamespace("n1", "urn:schemas-microsoft-com:asm.v3");
                manager.AddNamespace("n2", "http://schemas.microsoft.com/win/2004/08/events");

                XPathNodeIterator nodes = navigator.Select("/n1:assembly/n1:instrumentation/n2:events/n2:provider/n2:events/n2:event", manager);
                OutputSection(this.eventsSection, 10, nodes, "event", "channel", "level", "message", "opcode", "keywords", "symbol", "template", "task", "value", "version");

                nodes = navigator.Select("/n1:assembly/n1:instrumentation/n2:events/n2:provider/n2:tasks/n2:task", manager);
                OutputSection(this.tasksSection, 10, nodes, "task", "message", "name", "value");

                nodes = navigator.Select("/n1:assembly/n1:instrumentation/n2:events/n2:provider/n2:keywords/n2:keyword", manager);
                OutputSection(this.keywordsSection, 10, nodes, "keyword", "mask", "message", "name");

                nodes = navigator.Select("/n1:assembly/n1:instrumentation/n2:events/n2:provider/n2:templates/n2:template", manager);
                while (nodes.MoveNext())
                {
                    XPathNavigator dataNavigator = nodes.Current;
                    this.templatesSection.AppendFormat(CultureInfo.InvariantCulture, "          <template tid=\"{0}\">\r\n", dataNavigator.GetAttribute("tid", string.Empty));

                    if (dataNavigator.MoveToChild(XPathNodeType.Element))
                    {
                        do
                        {
                            OutputTag(this.templatesSection, 12, nodes.Current, "data", "inType", "name", "outType", "map");
                        }
                        while (dataNavigator.MoveToNext(XPathNodeType.Element));
                    }

                    this.templatesSection.AppendFormat(CultureInfo.InvariantCulture, "          </template>\r\n");
                }

                nodes = navigator.Select("/n1:assembly/n1:instrumentation/n2:events/n2:provider/n2:maps/n2:valueMap", manager);
                while (nodes.MoveNext())
                {
                    XPathNavigator dataNavigator = nodes.Current;
                    this.mapsSection.AppendFormat(CultureInfo.InvariantCulture, "          <valueMap name=\"{0}\">\r\n", dataNavigator.GetAttribute("name", string.Empty));

                    if (dataNavigator.MoveToChild(XPathNodeType.Element))
                    {
                        do
                        {
                            OutputTag(this.mapsSection, 12, nodes.Current, "map", "value", "message");
                        }
                        while (dataNavigator.MoveToNext(XPathNodeType.Element));
                    }

                    this.mapsSection.AppendFormat(CultureInfo.InvariantCulture, "          </valueMap>\r\n");
                }

                nodes = navigator.Select("/n1:assembly/n1:instrumentation/n2:events/n2:provider/n2:maps/n2:bitMap", manager);
                while (nodes.MoveNext())
                {
                    XPathNavigator dataNavigator = nodes.Current;
                    this.mapsSection.AppendFormat(CultureInfo.InvariantCulture, "          <bitMap name=\"{0}\">\r\n", dataNavigator.GetAttribute("name", string.Empty));

                    if (dataNavigator.MoveToChild(XPathNodeType.Element))
                    {
                        do
                        {
                            OutputTag(this.mapsSection, 12, nodes.Current, "map", "value", "message");
                        }
                        while (dataNavigator.MoveToNext(XPathNodeType.Element));
                    }

                    this.mapsSection.AppendFormat(CultureInfo.InvariantCulture, "          </bitMap>\r\n");
                }

                nodes = navigator.Select("/n1:assembly/n1:localization/n1:resources/n1:stringTable/n1:string", manager);
                OutputSection(this.resourcesSection, 8, nodes, "string", "id", "value");
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public void Write(string path, Guid guid, string name, string message, string symbol, string targetFile)
        {
            FileStream fileStream = File.Open(path, FileMode.Create);
            using (StreamWriter writer = new StreamWriter(fileStream, Encoding.UTF8))
            {
                writer.Write("<?xml version='1.0' encoding='utf-8' standalone='yes'?>\r\n");
                writer.Write("<assembly\r\n");
                writer.Write("    xmlns=\"urn:schemas-microsoft-com:asm.v3\"\r\n");
                writer.Write("    xmlns:test=\"http://manifests.microsoft.com/win/2004/08/test/events\"\r\n");
                writer.Write("    xmlns:win=\"http://manifests.microsoft.com/win/2004/08/windows/events\"\r\n");
                writer.Write("    xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"\r\n");
                writer.Write("    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n");
                writer.Write("    manifestVersion=\"1.0\"\r\n");
                writer.Write("    >\r\n");
                writer.Write("  <assemblyIdentity\r\n");
                writer.Write("      buildType=\"$(build.buildType)\"\r\n");
                writer.Write("      language=\"neutral\"\r\n");
                writer.Write("      name=\"Microsoft-ServiceFabric-Events\"\r\n");
                writer.Write("      processorArchitecture=\"$(build.processorArchitecture)\"\r\n");
                writer.Write("      publicKeyToken=\"$(Build.WindowsPublicKeyToken)\"\r\n");
                writer.Write("      version=\"$(build.version)\"\r\n");
                writer.Write("      versionScope=\"nonSxS\"\r\n");
                writer.Write("      />\r\n");
                writer.Write("  <instrumentation>\r\n");
                writer.Write("    <events\r\n");
                writer.Write("        xmlns=\"http://schemas.microsoft.com/win/2004/08/events\"\r\n");
                writer.Write("        xmlns:auto-ns1=\"urn:schemas-microsoft-com:asm.v3\"\r\n");
                writer.Write("        >\r\n");
                writer.Write("      <provider\r\n");
                writer.Write("          guid=\"{" + guid.ToString() + "}\"\r\n");
                writer.Write("          message=\"{0}\"\r\n", this.StringResource(message));
                writer.Write("          messageFileName=\"{0}\"\r\n", targetFile);
                writer.Write("          name=\"{0}\"\r\n", name);
                writer.Write("          resourceFileName=\"{0}\"\r\n", targetFile);
                writer.Write("          symbol=\"{0}\"\r\n", symbol);
                writer.Write("          >\r\n");

                this.WriteChannels(writer, symbol);

                this.WriteContents(writer);

                writer.Write("      </provider>\r\n");
                writer.Write("    </events>\r\n");
                writer.Write("  </instrumentation>\r\n");

                this.WriteResources(writer);

                writer.Write("</assembly>\r\n");
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private static void OutputTag(StringBuilder writer, int offset, XPathNavigator navigator, string name, params string[] attributes)
        {
            writer.AppendFormat(CultureInfo.InvariantCulture, "{0}<{1}", new string(' ', offset), name);
            foreach (string attribute in attributes)
            {
                string value = navigator.GetAttribute(attribute, string.Empty);
                if (!string.IsNullOrEmpty(value))
                {
                    writer.AppendFormat(CultureInfo.InvariantCulture, " {0}=\"{1}\"", attribute, value);
                }
            }

            writer.AppendFormat(CultureInfo.InvariantCulture, " />\r\n");
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private static void OutputSection(StringBuilder writer, int offset, XPathNodeIterator nodes, string name, params string[] attributes)
        {
            while (nodes.MoveNext())
            {
                OutputTag(writer, offset, nodes.Current, name, attributes);
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private void WriteContents(TextWriter writer)
        {
            writer.Write("        <events>\r\n");
            writer.Write(this.eventsSection.ToString());
            writer.Write("        </events>\r\n");

            writer.Write("        <tasks>\r\n");
            writer.Write(this.tasksSection.ToString());
            writer.Write("        </tasks>\r\n");

            writer.Write("        <templates>\r\n");
            writer.Write(this.templatesSection.ToString());
            writer.Write("        </templates>\r\n");

            writer.Write("        <maps>\r\n");
            writer.Write(this.mapsSection.ToString());
            writer.Write("        </maps>\r\n");

            if (this.keywordsSection.Length > 0)
            {
                writer.Write("        <keywords>\r\n");
                writer.Write(this.keywordsSection.ToString());
                writer.Write("        </keywords>\r\n");
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private void WriteChannels(TextWriter writer, string symbol)
        {
            writer.Write("        <channels>\r\n");
            string name = "Microsoft-ServiceFabric";

            foreach (EventChannel channel in Enum.GetValues(typeof(EventChannel)))
            {
                    writer.Write("          <channel\r\n");
                    writer.Write("              chid=\"{0}\"\r\n", channel);
                    if (channel == EventChannel.Debug ||
                        channel == EventChannel.Analytic)
                    {
                        writer.Write("              enabled=\"false\"\r\n");
                    }
                    else 
                    {
                    writer.Write("              enabled=\"true\"\r\n");
                    }

                    writer.Write("              isolation=\"Application\"\r\n");
                    writer.Write("              name=\"{0}/{1}\"\r\n", name, channel.ToString());
                    writer.Write("              symbol=\"CHANNEL_{0}_{1}\"\r\n", symbol, channel);
                    writer.Write("              type=\"{0}\"\r\n", channel);
                    writer.Write("              />\r\n");
            }

            writer.Write("        </channels>\r\n");
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        private void WriteResources(TextWriter writer)
        {
            writer.Write("  <localization>\r\n");
            writer.Write("    <resources culture=\"en-US\">\r\n");
            writer.Write("      <stringTable>\r\n");

            writer.Write(this.resourcesSection);

            foreach (uint hashValue in this.stringTable.Keys)
            {
                Dictionary<string, uint> sameHashStrings = this.stringTable[hashValue];
                foreach (string resource in sameHashStrings.Keys)
                {
                    uint differentiator = sameHashStrings[resource];
                    writer.Write("        <string\r\n");
                    writer.Write("            id=\"ms{0}.{1}\"\r\n", hashValue, differentiator);
                    writer.Write("            value=\"{0}\"\r\n", resource);
                    writer.Write("            />\r\n");
                }
            }

            writer.Write("      </stringTable>\r\n");
            writer.Write("    </resources>\r\n");
            writer.Write("  </localization>\r\n");
        }
    }
}