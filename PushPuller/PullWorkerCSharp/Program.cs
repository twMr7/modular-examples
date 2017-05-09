using System;
using System.Threading;
using System.Runtime.InteropServices;
using ZeroMQ;

namespace PullWorkerCSharp
{
    [ StructLayout(LayoutKind.Sequential) ]
    public struct Point3d
    {
        public double x;
        public double y;
        public double z;
    }

    class Program
    {
        const string Pusher_Address = "tcp://127.0.0.1:6866";

        static void Main(string[] args)
        {
            var error = default(ZError);
            using (var context = new ZContext())
            using (var puller = new ZSocket(context, ZSocketType.PULL))
            {
                Console.CancelKeyPress += (sender, eventargs) =>
                {
                    eventargs.Cancel = true;
                    context.Shutdown();
                };

                // connect puller
                try
                {
                    puller.Connect(Pusher_Address);
                }
                catch (ZException e)
                {
                    Console.WriteLine("failed connect to Dio: {0}", e.ToString());
                    return;
                }

                // try one receive and one send for every N msec.
                while (true)
                {
                    // try pull a job
                    ZMessage msgIncoming;
                    if (null == (msgIncoming = puller.ReceiveMessage(ZSocketFlags.DontWait, out error)))
                    {
                        if (error == ZError.EAGAIN)
                        {
                            Thread.Sleep(1);
                            continue;
                        }
                        if (error == ZError.ETERM)
                            break;  // Interrupted
                        throw new ZException(error);
                    }

                    //string strdata = msgIncoming.PopString();
                    Console.WriteLine("### New job:");

                    // 1st frame is the number of points
                    int numOfPoints = msgIncoming.PopInt32();

                    // 2nd frame is the Point3d structure array
                    ZFrame p3dFrame = msgIncoming.Pop();
                    Console.WriteLine("Contains {0} points, {1} bytes", numOfPoints, p3dFrame.Length);
                    IntPtr p3dPtr = p3dFrame.DataPtr();
                    Point3d[] p3data = new Point3d[numOfPoints];
                    Console.WriteLine(">>> Point3d array dump");
                    for (int i = 0; i < numOfPoints; ++i)
                    {
                        p3data[i] = (Point3d)Marshal.PtrToStructure(p3dPtr + (i * Marshal.SizeOf(typeof(Point3d))), typeof(Point3d));
                        Console.WriteLine("\t{0}, {1}, {2}", p3data[i].x, p3data[i].y, p3data[i].z);
                    }

                    // 3rd frame is the size of double array
                    uint sizeOfDoubleArray = msgIncoming.PopUInt32();

                    // 4th frame is the double array
                    ZFrame frameDoubleArray = msgIncoming.Pop();
                    Console.WriteLine("Contains {0} doubles, {1} bytes", sizeOfDoubleArray, frameDoubleArray.Length);
                    IntPtr dvecPtr = frameDoubleArray.DataPtr();
                    double[] dvec = new double[sizeOfDoubleArray];
                    Console.WriteLine(">>> double array dump");
                    for (int i = 0; i < sizeOfDoubleArray; ++i)
                    {
                        dvec[i] = (Double)Marshal.PtrToStructure(dvecPtr + (i * Marshal.SizeOf(typeof(Double))), typeof(Double));
                        Console.Write("{0}, ", dvec[i]);
                    }
                    Console.WriteLine();

                    // 5th frame is a double scalar
                    ZFrame frameDoubleScalar = msgIncoming.Pop();
                    IntPtr dscalarPtr = frameDoubleScalar.DataPtr();
                    double dscalar = (Double)Marshal.PtrToStructure(dscalarPtr, typeof(Double));
                    Console.WriteLine("A double scalar: {0}", dscalar);

                    Thread.Sleep(3);
                }

                if (error == ZError.ETERM)
                {
                    Console.WriteLine("Terminated! You have pressed CTRL+C.");
                    return;
                }
                throw new ZException(error);
            }
        }
    }
}
