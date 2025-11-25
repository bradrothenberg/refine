using nTop.Core.Wrapper;

namespace nTop.Core.Examples.Volume
{
    public partial class Volume
    {
        /// <summary>Computes the (approximate) volume of an implicit body by using voxels</summary>
        /// <param name="body">The body whose volume we want</param>
        /// <param name="n">Number of voxels used will be n*n*n</param>
        /// <returns>The computed volume</returns>
        public static double ComputeVolume(ImplicitBody body, int n)
        {
            // Get the body's bounding box
            Box3d box = body.Box;

            double dx = (box.MaxX - box.MinX) / n;
            double dy = (box.MaxY - box.MinY) / n;
            double dz = (box.MaxZ - box.MinZ) / n;

            int count = 0;

            // Loop to count number of voxels inside the body
            for (int i = 0; i <= n; i++)
            {
                for (int j = 0; j <= n; j++)
                {
                    for (int k = 0; k <= n; k++)
                    {
                        double x = box.MinX + i * dx + 0.5 * dx;
                        double y = box.MinY + j * dy + 0.5 * dy;
                        double z = box.MinZ + k * dz + 0.5 * dz;
                        double fxyz = body.Value(x, y, z);
                        if (fxyz < 0) count++;  // voxel center is inside body, so add to count
                    }
                }
            }

            // Compute volume = number of inside voxels * voxel volume
            double volume = count * (dx * dy * dz);

            return volume;
        }
    }
}
