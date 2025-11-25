
namespace nTop.Core.Examples.Mesher
{
    public class Triangle
    {
        /// <summary>The three vertices of the triangle, v0, v1, v2</summary>
        public Vector[] Vertices;

        /// <summary>Unit normal vector, in the direction of (v1 - v0) x (v2 - v1)</summary>
        public Vector UnitNormal;

        /// <summary>Constructor</summary>
        /// <param name="vertices">Vertices, in CCW order when viewed from outside the body</param>
        public Triangle(params Vector[] vertices)
        {
            this.Vertices = vertices;
            Vector v0 = vertices[0];
            Vector v1 = vertices[1];
            Vector v2 = vertices[2];
            this.UnitNormal = Vector.Cross(v1 - v0, v2 - v1).Unit;
        }
    }
}
