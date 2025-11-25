using System.Collections.Generic;
using System.IO;

namespace nTop.Core.Examples.Mesher
{
    public partial class Program
    {
        private static string VectorToSTL(Vector v)
        {
            string sx = v.X.ToString("F6");
            string sy = v.Y.ToString("F6");
            string sz = v.Z.ToString("F6");
            string space = "  ";
            return sx + space + sy + space + sz;
        }

        public static void WriteSTL(List<Triangle> triList, string filePath)
        {
            using (var writer = new StreamWriter(filePath))
            {
                writer.WriteLine("solid object");

                foreach (var triangle in triList)
                {
                    writer.WriteLine("  facet normal " + VectorToSTL(triangle.UnitNormal));
                    writer.WriteLine("    outer loop");

                    foreach (var vertex in triangle.Vertices)
                    {
                        writer.WriteLine("      vertex " + VectorToSTL(vertex));
                    }

                    writer.WriteLine("    endloop");
                    writer.WriteLine("  endfacet");
                }

                writer.WriteLine("endsolid object");
            }
        }
    }
}
