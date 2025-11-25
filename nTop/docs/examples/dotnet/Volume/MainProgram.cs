using nTop.Core.Examples.Utils;
using nTop.Core.Wrapper;

namespace nTop.Core.Examples.Volume
{
    public partial class Volume
    {
        public static void Main()
        {
            // Get .implicit file path from user
            string path = CLI.LoadPrompt();

            // Create an ImplicitBody object
            var body = new ImplicitBody(path);

            // Compute the volume of the body
            int numVoxels = 100;
            var volume = ComputeVolume(body, numVoxels);

            // Cleanup
            body.Release();

            // For spherecube, true volume = 1458.149;
            // For sphere, true volume = 523.599;
            System.Console.WriteLine("Computed volume  = " + volume.ToString());
        }
    }
}
