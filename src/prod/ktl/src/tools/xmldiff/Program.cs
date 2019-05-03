//-----------------------------------------------------------------------
// <copyright file="Program.cs" company="Microsoft">
//     (c)opyright Microsoft Corp 2016
// </copyright>
//-----------------------------------------------------------------------

// Simple XML differencing utility. 
// NOTE: Makes use of the XmlDiffPatch.XmlDiff class.
namespace XmlDiffTool
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Xml;
    using Microsoft.XmlDiffPatch;

    /// <summary>
    /// Main program
    /// </summary>
    public class Program
    {
        /// <summary>
        /// Entry point
        /// </summary>
        /// <param name="args">Command line args</param>
        /// <returns>Program results</returns>
        public static int Main(string[] args)
        {
            try
            {
                Console.WriteLine("XmlDiff - Compare two XML files");
                if (args.Length != 2)
                {
                    Console.WriteLine("Usage: XmlDiff <file1> <file2>");
                    return -1;
                }

                Console.WriteLine("\tAttemptng to diff: ({0}, {1})", args[0], args[1]);

                XmlDiff diff = new XmlDiff(XmlDiffOptions.IgnoreChildOrder | XmlDiffOptions.IgnoreComments | XmlDiffOptions.IgnoreWhitespace);
                XmlWriter diffgramWriter = new XmlTextWriter(args[0] + ".diffgram.xml", new System.Text.UnicodeEncoding());
                if (!diff.Compare(args[0], args[1], false, diffgramWriter))
                {
                    Console.WriteLine("\tFile did not compared exactly - for differences see {0}", args[0] + ".diffgram.xml");
                    return -2;
                }

                diffgramWriter.Flush();
                diffgramWriter.Close();
                Console.WriteLine("\tFile Compared Exactly");
                return 0;
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return -3;
            }
        }
    }
}
