﻿using Coral.Managed.Interop;
using Nuake.Net.Physics.Shapes;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace Nuake.Net
{

    namespace Physics
    {
        namespace Shapes
        {
            public class Box 
            {
                public float Width { get; set; } = 1.0f;
                public float Height { get; set; } = 1.0f;
                public float Depth { get; set; } = 1.0f;

                public Box(float width, float height, float depth) 
                {
                    Width = width;
                    Height = height;
                    Depth = depth;
                }

                public Box(Vector3 size)
                {
                    Width = size.X;
                    Height = size.Y;
                    Depth = size.Z;
                }

                public Vector3 GetSize()
                {
                    return new Vector3(Width, Height, Depth);
                }

                public void SetSize(Vector3 size)
                {
                    Width = size.X;
                    Height = size.Y;
                    Depth = size.Z;
                }
            }

            public class Sphere 
            {
                public float Radius { get; set; } = 0.5f;

                public Sphere(float radius)
                {
                    Radius = radius;
                }
            }

            public class Capsule 
            {
                public float Radius { get; set; } = 0.5f;
                public float Height { get; set; } = 1.0f;

                public Capsule(float radius, float height)
                {
                    Radius = radius;
                    Height = height;
                }
            }

            public class Cylinder
            {
                public float Radius { get; set; } = 0.5f;
                public float Height { get; set; } = 1.0f;

                public Cylinder(float radius, float height)
                {
                    Radius = radius;
                    Height = height;
                }
            }
        }
    }

    public class Physic
    {
        public struct ShapeCastResult
        {
            public Vector3 ImpactPosition;
            public Vector3 ImpactNormal;
            public float Fraction;
        }
        public struct BoxInternal
        {
            public float X;
            public float Y;
            public float Z;
        }

        public struct CapsuleInternal
        {
            public float Radius;
            public float Height;
        }

        internal static unsafe delegate*<float, float, float, float, float, float, BoxInternal, NativeArray<float>> ShapeCastBoxIcall;
        internal static unsafe delegate*<float, float, float, float, float, float, float, NativeArray<float>> ShapeCastSphereIcall;
        internal static unsafe delegate*<float, float, float, float, float, float, CapsuleInternal, NativeArray<float>> ShapeCastCapsuleIcall;
        internal static unsafe delegate*<float, float, float, float, float, float, CapsuleInternal, NativeArray<float>> ShapeCastCylinderIcall;
        public static List<ShapeCastResult> ShapeCast(Vector3 from, Vector3 to, Box box)
        {
            List<ShapeCastResult> result = [];

            BoxInternal boxInternal = new()
            {
                X = box.Width,
                Y = box.Height,
                Z = box.Depth
            };

            unsafe
            {
                NativeArray<float> resultICall = ShapeCastBoxIcall(from.X, from.Y, from.Z, to.X, to.Y, to.Z, boxInternal);

                
                for (int i = 0; i < resultICall.Length / 7; i++)
                {
                    int index = i * 7;
                    ShapeCastResult shapeCastResult = new()
                    {
                        ImpactPosition = new Vector3(resultICall[index + 0], resultICall[index + 1], resultICall[index + 2]),
                        ImpactNormal = new Vector3(resultICall[index + 3], resultICall[index + 4], resultICall[index + 5]),
                        Fraction = resultICall[index + 6]
                    };

                    result.Add(shapeCastResult);
                }
            }

            return result;
        }

        public static List<ShapeCastResult> ShapeCast(Vector3 from, Vector3 to, Sphere sphere)
        {
            List<ShapeCastResult> result = [];

            unsafe
            {
                NativeArray<float> resultICall = ShapeCastSphereIcall(from.X, from.Y, from.Z, to.X, to.Y, to.Z, sphere.Radius);

                for (int i = 0; i < resultICall.Length / 7; i++)
                {
                    int index = i * 7;
                    ShapeCastResult shapeCastResult = new()
                    {
                        ImpactPosition = new Vector3(resultICall[index + 0], resultICall[index + 1], resultICall[index + 2]),
                        ImpactNormal = new Vector3(resultICall[index + 3], resultICall[index + 4], resultICall[index + 5]),
                        Fraction = resultICall[index + 6]
                    };

                    result.Add(shapeCastResult);
                }
            }

            return result;
        }

        public static List<ShapeCastResult> ShapeCast(Vector3 from, Vector3 to, Capsule capsule)
        {
            List<ShapeCastResult> result = new List<ShapeCastResult>();

            CapsuleInternal capsuleInternal = new()
            {
                Radius = capsule.Radius,
                Height = capsule.Height
            };

            unsafe
            {
                NativeArray<float> resultICall = ShapeCastCapsuleIcall(from.X, from.Y, from.Z, to.X, to.Y, to.Z, capsuleInternal);

                for (int i = 0; i < resultICall.Length / 7; i++)
                {
                    int index = i * 7;
                    ShapeCastResult shapeCastResult = new()
                    {
                        ImpactPosition = new Vector3(resultICall[index + 0], resultICall[index + 1], resultICall[index + 2]),
                        ImpactNormal = new Vector3(resultICall[index + 3], resultICall[index + 4], resultICall[index + 5]),
                        Fraction = resultICall[index + 6]
                    };

                    result.Add(shapeCastResult);
                }
            }

            return result;
        }

        public static List<ShapeCastResult> ShapeCast(Vector3 from, Vector3 to, Cylinder cylinder)
        {
            List<ShapeCastResult> result = [];

            CapsuleInternal cylinderInternal = new()
            {
                Radius = cylinder.Radius,
                Height = cylinder.Height
            };

            unsafe
            {
                NativeArray<float> resultICall = ShapeCastCylinderIcall(from.X, from.Y, from.Z, to.X, to.Y, to.Z, cylinderInternal);

                for (int i = 0; i < resultICall.Length / 7; i++)
                {
                    int index = i * 7;
                    ShapeCastResult shapeCastResult = new()
                    {
                        ImpactPosition = new Vector3(resultICall[index + 0], resultICall[index + 1], resultICall[index + 2]),
                        ImpactNormal = new Vector3(resultICall[index + 3], resultICall[index + 4], resultICall[index + 5]),
                        Fraction = resultICall[index + 6]
                    };

                    result.Add(shapeCastResult);
                }
            }

            return result;
        }
    }
}
