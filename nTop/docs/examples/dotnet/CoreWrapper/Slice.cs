namespace nTop.Core.Wrapper
{
    public class Slice
    {
        /// <summary>
        /// List of planar contours represented as polylines comprizing the slice
        /// </summary>
        public List<List<Tuple<double, double>>> Contours { get; }

        /// <summary>
        /// Initialize a new slice
        /// </summary>
        public Slice()
        {
            Contours = new List<List<Tuple<double, double>>>();
        }
    }
}
