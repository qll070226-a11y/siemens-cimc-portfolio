using System;
using System.Text;

namespace CimcDebugTool
{
    internal static class ProtocolTestsFixed
    {
        private static int failures;
        private static void Check(bool value, string name)
        {
            if (value) Console.WriteLine("[OK] " + name);
            else { Console.WriteLine("[FAIL] " + name); failures++; }
        }

        public static int Main()
        {
            byte[] read = ModbusRtu.ReadRegisters(1, 4, 0, 6);
            Check(ByteUtil.ToHex(read) == "01 04 00 00 00 06 70 08", "Modbus read request and CRC");
            byte[] response = new byte[] { 1, 4, 12, 0x2E, 0xE0, 0x13, 0x88, 0x27, 0x10, 0, 2, 0, 1, 0, 2, 0, 0 };
            ushort crc = ByteUtil.Crc16Modbus(response, 0, 15); response[15] = (byte)crc; response[16] = (byte)(crc >> 8);
            ModbusResponse parsed = ModbusRtu.Parse(response, 1, 4);
            Check(parsed.Registers.Length == 6 && parsed.Registers[0] == 12000 && parsed.Registers[2] == 10000, "Modbus response parse");
            byte[] write = ModbusRtu.WriteSingle(1, 0, 100);
            Check(ByteUtil.ToHex(write) == "01 06 00 00 00 64 88 21", "Modbus write single CRC");
            byte[] legacy = InitialProtocol.BuildAscii(1, 1, 0x0104, null);
            Check(Encoding.ASCII.GetString(legacy).StartsWith("A5B600010101040002"), "Initial protocol header");
            byte[] ack = InitialProtocol.BuildAscii(1, 2, 0x0502, new byte[] { 0xFF });
            Check(InitialProtocol.IsAckOk(ack, 0x0502), "Initial protocol ACK validation");
            byte[] prepared = InitialProtocol.PrepareFirmware(new byte[] { 1, 2, 3 });
            Check(prepared.Length == 7 && InitialProtocol.HasFirmwareMagic(prepared) && prepared[4] == 1, "Firmware magic preparation");
            bool badHex = false; try { ByteUtil.ParseHex("01 GG"); } catch (FormatException) { badHex = true; }
            Check(badHex, "Reject malformed hex");
            Console.WriteLine(failures == 0 ? "All protocol tests passed." : failures + " test(s) failed.");
            return failures == 0 ? 0 : 1;
        }
    }
}
