using System.IO;
using System.Runtime.InteropServices;

namespace nTop.Core.Wrapper
{
    /// <summary>Class representing an nTop implicit body</summary>
    public class ImplicitBody
    {
        
        /// <summary>
        /// Defines where to find the nTop Core library.
        /// Examples for *nix and Windows are provided.
        /// </summary>
        /// For Windows
        private const string _dllLocation = "ntop_core.dll";
        // For *nix
        // private const string _dllLocation = "libntop_core.so.4.9.3";

        [DllImport(_dllLocation, EntryPoint = "ntop_core_import_from_file", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern bool GetTag(string path, out IntPtr tag);

        [DllImport(_dllLocation, EntryPoint = "ntop_core_release", CallingConvention = CallingConvention.Cdecl)]
        private static extern void ReleaseTag(IntPtr tag);

        [DllImport(_dllLocation, EntryPoint = "ntop_core_query_field", CallingConvention = CallingConvention.Cdecl)]
        private static extern double Query(IntPtr tag, Vec3 point);

        [DllImport(_dllLocation, EntryPoint = "ntop_core_query_bounding_box", CallingConvention = CallingConvention.Cdecl)]
        private static extern void GetBox(IntPtr tag, out BoundingBox box);

        // This is the signature of the callback used by ntop_core_query_contours
        // Two things of note: 
        // 1) This method of passing back a struct array leads to duplication of the data for increased readability.
        // 2) The user_ctx parameter is unused in this implementation as we can  
        private delegate void ContourCallbackDelegate(IntPtr user_ctx, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.Struct, SizeParamIndex = 2)] Vec2[] points, int point_count, bool is_closed);
        [DllImport(_dllLocation, EntryPoint = "ntop_core_query_contours", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Contour(IntPtr tag, ref Frame frame, double featureSize, IntPtr context, ContourCallbackDelegate cb);


        /// <summary>The tag (or handle) of the body</summary>
        private IntPtr Tag { get; }

        /// <summary>The axis-aligned bounding box of the body</summary>
        public Box3d Box
        {
            get
            {
                BoundingBox bb;
                GetBox(this.Tag, out bb);
                var box3d = new Box3d(bb.Min.X, bb.Min.Y, bb.Min.Z, bb.Max.X, bb.Max.Y, bb.Max.Z);
                return box3d;
            }
        }

        /// <summary>Constructor, given the pathname of a .implicit file</summary>
        /// <param name="path">Pathname of an .implicit file</param>
        public ImplicitBody(string path)
        {
            IntPtr tag = IntPtr.Zero;
            bool success = GetTag(path, out tag);
            this.Tag = tag;
        }

        /// <summary>
        /// Release this Implicit from memory
        /// </summary>
        public void Release()
        {
            ReleaseTag(this.Tag);
        }

        /// <summary>Computes the value of the body's implicit function</summary>
        /// <param name="xyz">The x,y,z coordinates of the point where we want to evaluate</param>
        /// <returns>The function value</returns>
        /// <remarks>The function value is negative inside the body, and positive outside</remarks>
        public double Value(params double[] xyz)
        {
            Vec3 point = new Vec3(xyz[0], xyz[1], xyz[2]);
            double fxyz = Query(this.Tag, point);
            return (double)fxyz;
        }

        /// <summary>Computes the value of the body's implicit function</summary>
        /// <param name="pt">The 3D point where we want to evaluate</param>
        /// <returns>The function value</returns>
        /// <remarks>The function value is negative inside the body, and positive outside</remarks>
        public double Value(Vector pt)
        {
            return this.Value(pt.X, pt.Y, pt.Z);
        }

        /// <summary>
        /// Extracts the slice data from contour callback by returning a function with access the caller's slice variable.
        /// </summary>
        /// <param name="s"></param>
        /// <returns></returns>
        private ContourCallbackDelegate contourCallback(Slice s)
        {
            // This is a closure matching the callback signature
            Action<IntPtr, Vec2[], int, bool> curveCallback = delegate (IntPtr user_ctx, Vec2[] points, int point_count, bool is_closed)
            {
                List<Tuple<double, double>> polyline = new List<Tuple<double, double>>();

                // Copy the points in the curve to the slice
                foreach (Vec2 v in points)
                {
                    polyline.Add(new Tuple<double, double>(v.X, v.Y));
                }

                // If the slice is closed add the first point back on.
                // This is mostly for rendering
                if (is_closed)
                {
                    polyline.Add(new Tuple<double, double>(points[0].X, points[0].Y));
                }

                s.Contours.Add(polyline);
            };

            return new ContourCallbackDelegate(curveCallback);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public Slice Slice(Frame frame, double featureSize)
        {
            Slice s = new Slice();

            ContourCallbackDelegate del = new ContourCallbackDelegate(contourCallback(s));

            Contour(this.Tag, ref frame, featureSize, IntPtr.Zero, del);

            return s;
        }
    }
}
