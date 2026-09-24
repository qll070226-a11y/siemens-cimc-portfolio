using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.Ports;
using System.Threading;

namespace CimcDebugTool
{
    internal sealed class SerialTransport : IDisposable
    {
        private readonly SerialPort port = new SerialPort();
        private readonly object gate = new object();
        public Action<string, byte[]> Trace;
        public bool IsOpen { get { return port.IsOpen; } }
        public string PortName { get { return port.PortName; } }

        public void Open(string name, int baud, Parity parity, int dataBits, StopBits stopBits)
        {
            lock (gate)
            {
                if (port.IsOpen) port.Close();
                port.PortName = name; port.BaudRate = baud; port.Parity = parity; port.DataBits = dataBits; port.StopBits = stopBits;
                port.Handshake = Handshake.None; port.ReadTimeout = 50; port.WriteTimeout = 1500;
                port.ReadBufferSize = 8192; port.WriteBufferSize = 8192; port.Open(); port.DiscardInBuffer(); port.DiscardOutBuffer();
            }
        }

        public void Close() { lock (gate) { if (port.IsOpen) port.Close(); } }

        public byte[] TransactModbus(byte[] request, int timeoutMs)
        {
            lock (gate)
            {
                EnsureOpen(); port.DiscardInBuffer(); port.Write(request, 0, request.Length); Emit("TX", request);
                List<byte> rx = new List<byte>(); Stopwatch sw = Stopwatch.StartNew(); int expected = -1;
                while (sw.ElapsedMilliseconds < timeoutMs)
                {
                    while (port.BytesToRead > 0)
                    {
                        rx.Add((byte)port.ReadByte()); expected = ModbusRtu.ExpectedLength(rx.ToArray());
                        if (expected > 0 && rx.Count >= expected) { byte[] done = rx.GetRange(0, expected).ToArray(); Emit("RX", done); return done; }
                    }
                    Thread.Sleep(2);
                }
                if (rx.Count > 0) Emit("RX", rx.ToArray());
                throw new TimeoutException("串口响应超时（" + timeoutMs + " ms）。");
            }
        }

        public byte[] TransactAscii(byte[] request, int timeoutMs)
        {
            lock (gate)
            {
                EnsureOpen(); port.DiscardInBuffer(); port.Write(request, 0, request.Length); Emit("TX", request);
                List<byte> rx = new List<byte>(); Stopwatch sw = Stopwatch.StartNew(); long last = 0;
                while (sw.ElapsedMilliseconds < timeoutMs)
                {
                    bool got = false;
                    while (port.BytesToRead > 0) { rx.Add((byte)port.ReadByte()); got = true; }
                    if (got) last = sw.ElapsedMilliseconds;
                    if (rx.Count > 0 && sw.ElapsedMilliseconds - last > 30) { byte[] done = rx.ToArray(); Emit("RX", done); return done; }
                    Thread.Sleep(2);
                }
                if (rx.Count > 0) { byte[] partial = rx.ToArray(); Emit("RX", partial); return partial; }
                throw new TimeoutException("串口响应超时（" + timeoutMs + " ms）。");
            }
        }

        public void WriteRaw(byte[] data, bool trace)
        {
            lock (gate) { EnsureOpen(); port.Write(data, 0, data.Length); if (trace) Emit("TX", data); }
        }

        public byte[] ReadUntilIdle(int timeoutMs, int idleMs)
        {
            lock (gate)
            {
                EnsureOpen(); List<byte> rx = new List<byte>(); Stopwatch sw = Stopwatch.StartNew(); long last = 0;
                while (sw.ElapsedMilliseconds < timeoutMs)
                {
                    bool got = false;
                    while (port.BytesToRead > 0) { rx.Add((byte)port.ReadByte()); got = true; }
                    if (got) last = sw.ElapsedMilliseconds;
                    if (rx.Count > 0 && sw.ElapsedMilliseconds - last >= idleMs) break;
                    Thread.Sleep(2);
                }
                byte[] done = rx.ToArray(); if (done.Length > 0) Emit("RX", done); return done;
            }
        }

        public void DiscardInput() { lock (gate) { EnsureOpen(); port.DiscardInBuffer(); } }
        private void EnsureOpen() { if (!port.IsOpen) throw new InvalidOperationException("请先打开串口。"); }
        private void Emit(string direction, byte[] data) { if (Trace != null) Trace(direction, data); }
        public void Dispose() { Close(); port.Dispose(); }
    }
}
