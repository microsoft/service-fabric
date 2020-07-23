// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AppManifestCleanupUtil
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Xml;
    using System.Xml.Linq;
    using System.Xml.Serialization;

    internal static class XmlSerializationUtility
    {
        public static string Serialize<T>(T value)
            where T : class
        {
            if (value == default(T))
            {
                return null;
            }

            var serializer = new XmlSerializer(typeof(T));

            using (var memoryStream = new MemoryStream())
            {
                using (var xmlWriter = XmlWriter.Create(memoryStream, GetXmlWriterSettings()))
                {
                    serializer.Serialize(xmlWriter, value);
                }

                memoryStream.Flush();
                return Encoding.UTF8.GetString(memoryStream.ToArray());
            }
        }

        public static T Deserialize<T>(string contents)
            where T : class
        {
            if (string.IsNullOrEmpty(contents))
            {
                return null;
            }

            var serializer = new XmlSerializer(typeof(T));
            using (var stringReader = new StringReader(contents))
            {
                var settings = new XmlReaderSettings
                {
                    XmlResolver = null,
                };

                using (var xmlReader = XmlReader.Create(stringReader, settings))
                {
                    return (T)serializer.Deserialize(xmlReader);
                }
            }
        }

        public static XmlWriterSettings GetXmlWriterSettings()
        {
            return new XmlWriterSettings
            {
                Encoding = new UTF8Encoding(false),
                Indent = true,
                OmitXmlDeclaration = false,
            };
        }

        /// <summary>
        /// Inserts Xml Comments from existingContent to xml obtained by serializing the object of type T.
        /// Exception while inserting comments are ignored and content without comments is returned.
        /// </summary>
        public static string InsertXmlComments<T>(string existingContent, T value)
            where T : class
        {
            var contentWithoutComments = XmlSerializationUtility.Serialize(value);

            try
            {
                return InsertXmlComments(existingContent, contentWithoutComments);
            }
            catch (Exception)
            {
                // Exception while inserting comments are ignored and content without comments is returned.
                return contentWithoutComments;
            }
        }

        /// <summary>
        /// Inserts Xml Comments from Xml content with comments into Xml content without comments.
        /// </summary>
        private static string InsertXmlComments(string contentWithComments, string contentWithoutComments)
        {
            var commentNodes = XDocument.Parse(contentWithComments).DescendantNodes().OfType<XComment>();
            var xdoc = XDocument.Parse(contentWithoutComments);

            // if there are no comments to insert, just return the contentsWithoutComments
            if (!commentNodes.Any())
            {
                return contentWithoutComments;
            }

            foreach (var commentNode in commentNodes)
            {
                var newComment = new XComment(commentNode.Value);

                // Handle comments at beginning and end of xml
                if (commentNode.Parent == null)
                {
                    // Handle comments at beginning of xml
                    if (commentNode.PreviousNode == null && commentNode.NextNode != null)
                    {
                        xdoc.Root.AddBeforeSelf(newComment);
                    }

                    // Handle comments at end of xml
                    if (commentNode.NextNode == null && commentNode.PreviousNode != null)
                    {
                        xdoc.Root.AddAfterSelf(newComment);
                    }
                }
                else
                {
                    // If Parent node of comment is not found in new xml, then dont add this comment.
                    var parent = xdoc.Descendants(commentNode.Parent.Name).FirstOrDefault();
                    if (parent != null)
                    {
                        var hadPrevious = commentNode.ElementsBeforeSelf().Any();
                        var hadNext = commentNode.ElementsAfterSelf().Any();

                        if (!hadNext)
                        {
                            // Handles if comment is last child of parent.
                            // Also handles if comment had no siblings i.e. node only has this comment.
                            parent.Add(newComment);
                        }
                        else if (!hadPrevious)
                        {
                            // Handles if comment is first child of parent.
                            parent.AddFirst(newComment);
                        }
                        else if (hadPrevious && hadNext)
                        {
                            // Handles if comment had both Previous and Next siblings.

                            // If PreviousNode of comment is in new Xml, add newComment after PreviousNode
                            // If PreviousNode is not in new xml and NextNode is in newXml, add newComment before NextNode.
                            // If both PreviousNode and NextNode is not in new xml skip it.
                            var previousInOriginal = commentNode.ElementsBeforeSelf().Last();
                            var nextInOriginal = commentNode.ElementsAfterSelf().First();

                            // Find matching Previous and Next siblings in new xml. Siblings must match in attributes as well.
                            var previous = FindNodeInXDocument(xdoc, previousInOriginal);
                            var next = FindNodeInXDocument(xdoc, nextInOriginal);

                            if (previous != null)
                            {
                                previous.AddAfterSelf(newComment);
                            }
                            else if (next != null)
                            {
                                next.AddBeforeSelf(newComment);
                            }
                        }
                    }
                }
            }

            // Use XmlWriterSettings same as used in serialization to keep changes to existing files to minimal.
            var stringBuilder = new StringBuilder();
            using (var memoryStream = new MemoryStream())
            {
                using (var xmlWriter = XmlWriter.Create(memoryStream, XmlSerializationUtility.GetXmlWriterSettings()))
                {
                    xdoc.Save(xmlWriter);
                    xmlWriter.Flush();
                    memoryStream.Flush();
                    return Encoding.UTF8.GetString(memoryStream.ToArray());
                }
            }
        }

        private static XElement FindNodeInXDocument(XDocument xdoc, XElement nodeToFind)
        {
            // Find the correct node in xDoc, it must match attributes as well.
            return xdoc.Descendants(nodeToFind.Name).Where(
                node =>
                {
                    if (node.Attributes().Count() != nodeToFind.Attributes().Count())
                    {
                        return false;
                    }

                    foreach (var attribOriginal in nodeToFind.Attributes())
                    {
                        var attrib = node.Attribute(attribOriginal.Name);

                        if (attrib == null)
                        {
                            return false;
                        }

                        if (!attrib.Value.Equals(attribOriginal.Value))
                        {
                            return false;
                        }
                    }

                    return true;
                }).FirstOrDefault();
        }
    }
}
