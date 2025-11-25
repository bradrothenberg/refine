using System;
using System.IO;

using nTop.Core.Examples.Utils;
using nTop.Core.Wrapper;

namespace nTop.Core.Examples.Mesher
{
    public partial class Program
    {
        static void Main()
        {
            // Get .implicit file path from user
            string path = CLI.LoadPrompt();

            // Create an ImplicitBody object
            var body = new ImplicitBody(path);

            // Invent some reasonable grid counts (number of voxels)
            int n = 200;
            int[] gridCounts = Mesher.GetGridCounts(body, n);

            // Make a box that's 10% larger than body's bounding box
            var boundingBox = body.Box;
            var paddedBox = boundingBox.Enlarge(1.1);

            // Do the meshing
            var triList = Mesher.CreateMesh(body, paddedBox, gridCounts);

            // Free the memory used by the implicit in the library
            body.Release();

            // Write out an STL file and display it in default viewer
            string meshFilePath = $"{Directory.GetCurrentDirectory()}{Path.DirectorySeparatorChar}mesher.stl";
            WriteSTL(triList, meshFilePath);
            System.Console.WriteLine($"Mesh data written to {meshFilePath}");
        }
    }
}
