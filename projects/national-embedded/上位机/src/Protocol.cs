using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace CimcDebugTool
{
    internal static class ByteUtil
    {
        public static ushort Crc16Modbus(byte[] data, int offset, int count)
        {
            ushort crc = 0xFFFF;
            for (int i = offset; i < offset + count; i++)
            {
                crc ^= data[i];
                for (int bit = 0; bit < 8; bit++)
                    crc = (ushort)(((crc & 1) != 0) ? ((crc >> 1) ^ 0xA001) : (crc >> 1));
            }
            return crc;
        }

        public static string ToHex(byte[] data)
        {
            if (data == null) return "";
            StringBuilder sb = new StringBuilder(data.Length * 3);
            for (int i = 0; i < data.Length; i++)
            {
                if (i > 0) sb.Append(' ');
                sb.Append(data[i].ToString("X2", CultureInfo.InvariantCulture));
            }
            return sb.ToString();
        }

        public static byte[] ParseHex(string text)
        {
            if (text == null) return new byte[0];
            StringBuilder clean = new StringBuilder();
            foreach (char ch in text)
            {
                if (Uri.IsHexDigit(ch)) clean.Append(ch);
                else if (!char.IsWhiteSpace(ch) && ch != '-' && ch != ',' && ch != ':')
                    throw new FormatException("十六进制包含非法字符：" + ch);
            }
            if ((clean.Length & 1) != 0) throw new FormatException("十六进制字符数量必须为偶数。");
            byte[] result = new byte[clean.Length / 2];
            for (int i = 0; i < result.Length; i++)
                result[i] = byte.Parse(clean.ToString(i * 2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            return result;
        }

        public static ushort Be16(byte[] p, int i) { return (ushort)((p[i] << 8) | p[i + 1]); }
        public static uint Be32(byte[] p, int i) { return ((uint)p[i] << 24) | ((uint)p[i + 1] << 16) | ((uint)p[i + 2] << 8) | p[i + 3]; }
        public static void PutBe16(List<byte> p, ushort value) { p.Add((byte)(value >> 8)); p.Add((byte)value); }
    }

    internal sealed class ModbusResponse
    {
        public byte Address;
        public byte Function;
        public ushort[] Registers = new ushort[0];
        public byte[] Raw = new byte[0];
        public byte? ExceptionCode;
        public bool IsException { get { return ExceptionCode.HasValue; } }
    }

    internal static class ModbusRtu
    {
        public static byte[] ReadRegisters(byte slave, byte function, ushort start, ushort count)
        {
            if (function != 3 && function != 4) throw new ArgumentException("读寄存器功能码必须是 03 或 04。");
            if (count < 1 || count > 125) throw new ArgumentOutOfRangeException("count");
            List<byte> frame = new List<byte>();
            frame.Add(slave); frame.Add(function); ByteUtil.PutBe16(frame, start); ByteUtil.PutBe16(frame, count);
            AddCrc(frame); return frame.ToArray();
        }

        public static byte[] WriteSingle(byte slave, ushort address, ushort value)
        {
            List<byte> frame = new List<byte>();
            frame.Add(slave); frame.Add(6); ByteUtil.PutBe16(frame, address); ByteUtil.PutBe16(frame, value);
            AddCrc(frame); return frame.ToArray();
        }

        public static byte[] WriteMultiple(byte slave, ushort address, ushort[] values)
        {
            if (values == null || values.Length < 1 || values.Length > 123) throw new ArgumentOutOfRangeException("values");
            List<byte> frame = new List<byte>();
            frame.Add(slave); frame.Add(16); ByteUtil.PutBe16(frame, address); ByteUtil.PutBe16(frame, (ushort)values.Length);
            frame.Add((byte)(values.Length * 2));
            foreach (ushort value in values) ByteUtil.PutBe16(frame, value);
            AddCrc(frame); return frame.ToArray();
        }

        public static ModbusResponse Parse(byte[] frame, byte expectedSlave, byte expectedFunction)
        {
            if (frame == null || frame.Length < 5) throw new InvalidOperationException("响应长度不足。");
            ushort calc = ByteUtil.Crc16Modbus(frame, 0, frame.Length - 2);
            ushort got = (ushort)(frame[frame.Length - 2] | (frame[frame.Length - 1] << 8));
            if (calc != got) throw new InvalidOperationException("响应 CRC 错误。");
            if (frame[0] != expectedSlave) throw new InvalidOperationException("响应从站地址不匹配。");
            ModbusResponse r = new ModbusResponse(); r.Address = frame[0]; r.Function = frame[1]; r.Raw = frame;
            if (frame[1] == (byte)(expectedFunction | 0x80)) { r.ExceptionCode = frame[2]; return r; }
            if (frame[1] != expectedFunction) throw new InvalidOperationException("响应功能码不匹配。");
            if (expectedFunction == 3 || expectedFunction == 4)
            {
                int bytes = frame[2];
                if ((bytes & 1) != 0 || frame.Length != bytes + 5) throw new InvalidOperationException("响应字节数错误。");
                r.Registers = new ushort[bytes / 2];
                for (int i = 0; i < r.Registers.Length; i++) r.Registers[i] = ByteUtil.Be16(frame, 3 + i * 2);
            }
            return r;
        }

        public static int ExpectedLength(byte[] received)
        {
            if (received == null || received.Length < 2) return -1;
            if ((received[1] & 0x80) != 0) return 5;
            if (received[1] == 3 || received[1] == 4)
                return received.Length >= 3 ? received[2] + 5 : -1;
            if (received[1] == 6 || received[1] == 16) return 8;
            return -1;
        }

        private static void AddCrc(List<byte> frame)
        {
            byte[] raw = frame.ToArray(); ushort crc = ByteUtil.Crc16Modbus(raw, 0, raw.Length);
            frame.Add((byte)crc); frame.Add((byte)(crc >> 8));
        }
    }

    internal static class InitialProtocol
    {
        public const ushort UpgradeRequest = 0x0501;
        public const ushort UpgradePrepare = 0x0502;
        public const ushort UpgradeExecute = 0x0503;

        public static byte[] BuildAscii(ushort deviceId, byte type, ushort command, byte[] payload)
        {
            payload = payload ?? new byte[0];
            if (payload.Length > 64) throw new ArgumentOutOfRangeException("payload");
            List<byte> raw = new List<byte>();
            raw.Add(0xA5); raw.Add(0xB6); ByteUtil.PutBe16(raw, deviceId); raw.Add(type); ByteUtil.PutBe16(raw, command);
            raw.Add((byte)payload.Length); raw.Add(0x02); raw.AddRange(payload);
            ushort crc = ByteUtil.Crc16Modbus(raw.ToArray(), 0, raw.Count);
            raw.Add((byte)(crc >> 8)); raw.Add((byte)crc); raw.Add(0xB6); raw.Add(0xA5);
            return Encoding.ASCII.GetBytes(BitConverter.ToString(raw.ToArray()).Replace("-", ""));
        }

        public static bool HasFirmwareMagic(byte[] data)
        {
            return data != null && data.Length >= 4 && data[0] == 0x5A && data[1] == 0xA5 && data[2] == 0xC3 && data[3] == 0x3C;
        }

        public static byte[] PrepareFirmware(byte[] data)
        {
            if (data == null) throw new ArgumentNullException("data");
            if (HasFirmwareMagic(data)) return data;
            byte[] result = new byte[data.Length + 4];
            result[0] = 0x5A; result[1] = 0xA5; result[2] = 0xC3; result[3] = 0x3C;
            Buffer.BlockCopy(data, 0, result, 4, data.Length);
            return result;
        }

        public static bool IsAckOk(byte[] ascii, ushort command)
        {
            if (ascii == null) return false;
            string s = Encoding.ASCII.GetString(ascii).Trim();
            byte[] raw;
            try { raw = ByteUtil.ParseHex(s); } catch { return false; }
            if (raw.Length < 14 || raw[0] != 0xA5 || raw[1] != 0xB6 || raw[4] != 0x02) return false;
            if (ByteUtil.Be16(raw, 5) != command || raw[7] != 1 || raw[9] != 0xFF) return false;
            int crcOffset = 9 + raw[7];
            ushort calc = ByteUtil.Crc16Modbus(raw, 0, crcOffset);
            return ByteUtil.Be16(raw, crcOffset) == calc && ByteUtil.Be16(raw, crcOffset + 2) == 0xB6A5;
        }
    }
}
