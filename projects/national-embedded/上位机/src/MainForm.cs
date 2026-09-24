using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace CimcDebugTool
{
    internal sealed class MainForm : Form
    {
        private readonly SerialTransport transport = new SerialTransport();
        private readonly object operationGate = new object();
        private bool operationBusy;
        private bool monitorRunning;
        private CancellationTokenSource monitorCts;
        private byte monitorSlave;
        private int monitorTimeout;
        private int monitorPeriod;
        private bool simulated;
        private uint simulationCount;
        private readonly Random random = new Random(2026);

        private ComboBox cbPort, cbBaud, cbParity, cbStop, cbSlave;
        private Button btnRefresh, btnConnect, btnSimulate;
        private Label lblConnection, lblPollState;
        private NumericUpDown numTimeout, numPoll;
        private Label valCurrent, valVoltage1, valVoltage2, valCount;
        private Label badgeWire, badgeTfReady, badgeTfError, lblAdcRaw;
        private Button btnMonitor;
        private CheckBox chkCsv;
        private TextBox txtCsvPath;
        private Button btnCsvPath;
        private DataGridView gridMonitor;
        private Chart monitorChart;
        private ComboBox cbChartPoints;
        private CheckBox chkChartFrozen;
        private Label lblChartSamples;
        private readonly List<MeasurementPoint> measurementHistory = new List<MeasurementPoint>();
        private const int MaxHistoryPoints = 5000;

        private ComboBox cbRegType, cbFunction, cbValueFormat;
        private NumericUpDown numStart, numCount;
        private TextBox txtWriteValues;
        private DataGridView gridRegisters;
        private Button btnRead, btnWrite, btnScan;

        private TextBox txtRawTx, txtRawRx;
        private CheckBox chkAppendCrc;
        private Button btnRawSend, btnRawCrc;

        private NumericUpDown numLegacyId, numLegacyCmd;
        private TextBox txtLegacyPayload, txtLegacyBuilt, txtFirmware;
        private Button btnLegacyBuild, btnLegacySend, btnFirmwareBrowse, btnUpgrade;
        private ProgressBar upgradeProgress;
        private Label lblUpgrade;
        private CheckBox chkUpgradeRisk;

        private RichTextBox txtLog;
        private ToolStripStatusLabel statusText, statusCounters;
        private int txFrames, rxFrames, errorFrames;

        private static readonly Color Ink = Color.FromArgb(31, 40, 48);
        private static readonly Color Blue = Color.FromArgb(22, 103, 161);
        private static readonly Color Green = Color.FromArgb(31, 125, 90);
        private static readonly Color Red = Color.FromArgb(170, 55, 55);
        private static readonly Color Amber = Color.FromArgb(180, 113, 24);
        private static readonly Color Panel = Color.White;
        private static readonly Font UiFont = new Font("Microsoft YaHei UI", 9F);

        private sealed class MeasurementPoint
        {
            public DateTime Time;
            public double Current;
            public double Voltage1;
            public double Voltage2;
            public double Adc0Mv;
            public double Adc1Mv;
            public double Adc2Mv;
            public ushort Status;
            public uint Count;
        }

        public MainForm()
        {
            Text = "CIMC 2026 比赛调试上位机";
            StartPosition = FormStartPosition.CenterScreen;
            MinimumSize = new Size(1060, 720);
            Size = new Size(1280, 820);
            Font = UiFont;
            BackColor = Color.FromArgb(243, 246, 248);
            Icon = SystemIcons.Application;
            BuildUi();
            transport.Trace = OnTrace;
            RefreshPorts();
            FormClosing += OnFormClosing;
        }

        private void BuildUi()
        {
            TableLayoutPanel root = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 4, ColumnCount = 1, Padding = new Padding(10) };
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 62));
            root.RowStyles.Add(new RowStyle(SizeType.Percent, 72));
            root.RowStyles.Add(new RowStyle(SizeType.Percent, 28));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 24));
            Controls.Add(root);
            root.Controls.Add(BuildConnectionBar(), 0, 0);
            TabControl tabs = new TabControl { Dock = DockStyle.Fill, Padding = new Point(14, 5) };
            tabs.TabPages.Add(BuildMonitorTab()); tabs.TabPages.Add(BuildRegisterTab()); tabs.TabPages.Add(BuildRawTab()); tabs.TabPages.Add(BuildLegacyTab());
            root.Controls.Add(tabs, 0, 1);
            root.Controls.Add(BuildLogPanel(), 0, 2);
            StatusStrip strip = new StatusStrip { SizingGrip = false, BackColor = Color.White };
            statusText = new ToolStripStatusLabel("就绪") { Spring = true, TextAlign = ContentAlignment.MiddleLeft };
            statusCounters = new ToolStripStatusLabel("TX 0  RX 0  ERR 0");
            strip.Items.Add(statusText); strip.Items.Add(statusCounters); root.Controls.Add(strip, 0, 3);
        }

        private Control BuildConnectionBar()
        {
            Panel p = CardPanel();
            FlowLayoutPanel f = new FlowLayoutPanel { Dock = DockStyle.Fill, WrapContents = false, Padding = new Padding(10, 9, 8, 4), AutoScroll = true };
            p.Controls.Add(f);
            f.Controls.Add(Caption("串口")); cbPort = Combo(95); f.Controls.Add(cbPort);
            btnRefresh = ButtonOf("刷新", 58); btnRefresh.Click += delegate { RefreshPorts(); }; f.Controls.Add(btnRefresh);
            f.Controls.Add(Caption("波特率")); cbBaud = Combo(88); cbBaud.Items.AddRange(new object[] { "4800", "9600", "19200", "38400", "57600", "115200" }); cbBaud.Text = "19200"; f.Controls.Add(cbBaud);
            cbParity = Combo(72); cbParity.Items.AddRange(new object[] { "None", "Even", "Odd" }); cbParity.Text = "None"; f.Controls.Add(cbParity);
            cbStop = Combo(58); cbStop.Items.AddRange(new object[] { "1", "2" }); cbStop.Text = "1"; f.Controls.Add(cbStop);
            f.Controls.Add(Caption("从站")); cbSlave = Combo(54); for (int i = 1; i <= 247; i++) cbSlave.Items.Add(i); cbSlave.Text = "1"; f.Controls.Add(cbSlave);
            f.Controls.Add(Caption("超时")); numTimeout = Number(100, 10000, 1000, 70); numTimeout.Increment = 100; f.Controls.Add(numTimeout); f.Controls.Add(Caption("ms"));
            btnConnect = AccentButton("打开串口", 84); btnConnect.Click += ToggleConnection; f.Controls.Add(btnConnect);
            btnSimulate = ButtonOf("离线模拟", 78); btnSimulate.Click += ToggleSimulation; f.Controls.Add(btnSimulate);
            lblConnection = new Label { Text = "未连接", AutoSize = true, ForeColor = Red, Font = new Font(UiFont, FontStyle.Bold), Padding = new Padding(7, 7, 0, 0) }; f.Controls.Add(lblConnection);
            return p;
        }

        private TabPage BuildMonitorTab()
        {
            TabPage page = new TabPage("实时监控") { BackColor = BackColor, Padding = new Padding(8) };
            TableLayoutPanel t = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 3, ColumnCount = 1 };
            t.RowStyles.Add(new RowStyle(SizeType.Absolute, 100)); t.RowStyles.Add(new RowStyle(SizeType.Absolute, 48)); t.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); page.Controls.Add(t);
            FlowLayoutPanel cards = new FlowLayoutPanel { Dock = DockStyle.Fill, WrapContents = false, AutoScroll = true };
            valCurrent = ValueCard(cards, "电流", "-- mA", Blue); valVoltage1 = ValueCard(cards, "电压 1", "-- V", Green); valVoltage2 = ValueCard(cards, "电压 2", "-- V", Amber); valCount = ValueCard(cards, "采样计数", "--", Ink);
            badgeWire = Badge(cards, "断线 --"); badgeTfReady = Badge(cards, "TF --"); badgeTfError = Badge(cards, "存储 --"); t.Controls.Add(cards, 0, 0);
            FlowLayoutPanel bar = new FlowLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(4, 6, 0, 3), WrapContents = false, AutoScroll = true };
            btnMonitor = AccentButton("开始轮询", 90); btnMonitor.Click += ToggleMonitor; bar.Controls.Add(btnMonitor);
            bar.Controls.Add(Caption("周期")); numPoll = Number(100, 60000, 500, 78); numPoll.Increment = 100; bar.Controls.Add(numPoll); bar.Controls.Add(Caption("ms"));
            chkCsv = new CheckBox { Text = "同时保存 CSV", AutoSize = true, Padding = new Padding(8, 5, 0, 0) }; bar.Controls.Add(chkCsv);
            txtCsvPath = new TextBox { Width = 300, Text = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "logs", "monitor.csv"), Margin = new Padding(5, 3, 2, 3) }; bar.Controls.Add(txtCsvPath);
            btnCsvPath = ButtonOf("选择", 58); btnCsvPath.Click += SelectCsv; bar.Controls.Add(btnCsvPath);
            lblPollState = new Label { Text = "停止", AutoSize = true, ForeColor = Color.DimGray, Padding = new Padding(10, 6, 0, 0) }; bar.Controls.Add(lblPollState);
            lblAdcRaw = new Label { Text = "ADC原始: AIN0 -- / AIN1 -- / AIN2 -- mV", AutoSize = true, ForeColor = Ink, Font = new Font(UiFont, FontStyle.Bold), Padding = new Padding(12, 6, 0, 0) }; bar.Controls.Add(lblAdcRaw); t.Controls.Add(bar, 0, 1);
            gridMonitor = NewGrid(); gridMonitor.Columns.Add("time", "时间"); gridMonitor.Columns.Add("current", "电流 mA"); gridMonitor.Columns.Add("v1", "电压1 V"); gridMonitor.Columns.Add("v2", "电压2 V"); gridMonitor.Columns.Add("adc0", "AIN0 mV"); gridMonitor.Columns.Add("adc1", "AIN1 mV"); gridMonitor.Columns.Add("adc2", "AIN2 mV"); gridMonitor.Columns.Add("status", "状态"); gridMonitor.Columns.Add("count", "采样计数");
            TabControl views = new TabControl { Dock = DockStyle.Fill, Padding = new Point(12, 4) };
            TabPage trendPage = new TabPage("趋势曲线") { BackColor = Color.White, Padding = new Padding(4) };
            trendPage.Controls.Add(BuildTrendPanel());
            TabPage tablePage = new TabPage("数据表") { BackColor = Color.White, Padding = new Padding(4) };
            tablePage.Controls.Add(gridMonitor);
            views.TabPages.Add(trendPage); views.TabPages.Add(tablePage); t.Controls.Add(views, 0, 2);
            return page;
        }

        private Control BuildTrendPanel()
        {
            TableLayoutPanel t = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 2, ColumnCount = 1, BackColor = Color.White };
            t.RowStyles.Add(new RowStyle(SizeType.Absolute, 40)); t.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            FlowLayoutPanel tools = new FlowLayoutPanel { Dock = DockStyle.Fill, WrapContents = false, AutoScroll = true, Padding = new Padding(4, 4, 0, 2) };
            chkChartFrozen = new CheckBox { Text = "冻结曲线", AutoSize = true, Padding = new Padding(3, 5, 6, 0) };
            chkChartFrozen.CheckedChanged += delegate { if (!chkChartFrozen.Checked) RebuildChart(); };
            tools.Controls.Add(chkChartFrozen); tools.Controls.Add(Caption("显示点数"));
            cbChartPoints = Combo(76); cbChartPoints.DropDownStyle = ComboBoxStyle.DropDownList; cbChartPoints.Items.AddRange(new object[] { "60", "120", "300", "1000" }); cbChartPoints.SelectedItem = "300"; cbChartPoints.SelectedIndexChanged += delegate { RebuildChart(); }; tools.Controls.Add(cbChartPoints);
            Button clear = ButtonOf("清空数据", 78); clear.Click += delegate { ClearMonitorData(); }; tools.Controls.Add(clear);
            Button import = ButtonOf("导入 CSV", 82); import.Click += ImportMonitorCsv; tools.Controls.Add(import);
            Button export = ButtonOf("导出曲线图", 92); export.Click += ExportChartImage; tools.Controls.Add(export);
            lblChartSamples = new Label { Text = "缓存 0 / 5000", AutoSize = true, ForeColor = Color.DimGray, Padding = new Padding(10, 7, 0, 0) }; tools.Controls.Add(lblChartSamples);
            t.Controls.Add(tools, 0, 0);

            monitorChart = new Chart { Dock = DockStyle.Fill, BackColor = Color.White, AntiAliasing = AntiAliasingStyles.All, TextAntiAliasingQuality = TextAntiAliasingQuality.High };
            ChartArea area = new ChartArea("趋势") { BackColor = Color.White };
            area.AxisX.LabelStyle.Format = "HH:mm:ss"; area.AxisX.MajorGrid.LineColor = Color.FromArgb(232, 237, 241); area.AxisX.Title = "PC 时间";
            area.AxisY.MajorGrid.LineColor = Color.FromArgb(232, 237, 241); area.AxisY.Title = "电流 (mA)"; area.AxisY.IsStartedFromZero = false;
            area.AxisY2.Enabled = AxisEnabled.True; area.AxisY2.Title = "电压 (V)"; area.AxisY2.IsStartedFromZero = false; area.AxisY2.MajorGrid.Enabled = false;
            monitorChart.ChartAreas.Add(area);
            Legend legend = new Legend("图例") { Docking = Docking.Top, Alignment = StringAlignment.Center, BackColor = Color.White };
            monitorChart.Legends.Add(legend);
            monitorChart.Series.Add(NewTrendSeries("电流", Blue, AxisType.Primary));
            monitorChart.Series.Add(NewTrendSeries("电压 1", Green, AxisType.Secondary));
            monitorChart.Series.Add(NewTrendSeries("电压 2", Amber, AxisType.Secondary));
            t.Controls.Add(monitorChart, 0, 1); return t;
        }

        private static Series NewTrendSeries(string name, Color color, AxisType axis)
        {
            return new Series(name) { ChartType = SeriesChartType.FastLine, BorderWidth = 2, Color = color, XValueType = ChartValueType.DateTime, YValueType = ChartValueType.Double, YAxisType = axis, ChartArea = "趋势", Legend = "图例" };
        }

        private TabPage BuildRegisterTab()
        {
            TabPage page = new TabPage("通用寄存器") { BackColor = BackColor, Padding = new Padding(8) };
            TableLayoutPanel t = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 2, ColumnCount = 1 }; t.RowStyles.Add(new RowStyle(SizeType.Absolute, 92)); t.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); page.Controls.Add(t);
            Panel top = CardPanel(); FlowLayoutPanel f = new FlowLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(9), AutoScroll = true };
            top.Controls.Add(f); f.Controls.Add(Caption("类型")); cbRegType = Combo(88); cbRegType.Items.AddRange(new object[] { "输入寄存器", "保持寄存器" }); cbRegType.SelectedIndex = 0; cbRegType.SelectedIndexChanged += delegate { SyncRegType(); }; f.Controls.Add(cbRegType);
            f.Controls.Add(Caption("功能码")); cbFunction = Combo(65); cbFunction.Items.AddRange(new object[] { "04", "03", "06", "10" }); cbFunction.Text = "04"; f.Controls.Add(cbFunction);
            f.Controls.Add(Caption("起始地址(0基)")); numStart = Number(0, 65535, 0, 82); f.Controls.Add(numStart);
            f.Controls.Add(Caption("数量")); numCount = Number(1, 125, 6, 62); f.Controls.Add(numCount);
            f.Controls.Add(Caption("显示")); cbValueFormat = Combo(70); cbValueFormat.Items.AddRange(new object[] { "DEC", "HEX", "有符号" }); cbValueFormat.Text = "DEC"; f.Controls.Add(cbValueFormat);
            btnRead = AccentButton("读取", 65); btnRead.Click += ReadGeneric; f.Controls.Add(btnRead);
            btnScan = ButtonOf("扫描地址", 78); btnScan.Click += ScanSlaves; f.Controls.Add(btnScan);
            f.SetFlowBreak(btnScan, true); f.Controls.Add(Caption("写入值（十进制/0x十六进制，多个以空格或逗号分隔）")); txtWriteValues = new TextBox { Width = 370, Text = "100", Margin = new Padding(4) }; f.Controls.Add(txtWriteValues); btnWrite = ButtonOf("写入", 65); btnWrite.Click += WriteGeneric; f.Controls.Add(btnWrite);
            t.Controls.Add(top, 0, 0); gridRegisters = NewGrid(); gridRegisters.Columns.Add("offset", "PDU 地址"); gridRegisters.Columns.Add("human", "人类编号"); gridRegisters.Columns.Add("dec", "无符号"); gridRegisters.Columns.Add("signed", "有符号"); gridRegisters.Columns.Add("hex", "十六进制"); t.Controls.Add(gridRegisters, 0, 1);
            return page;
        }

        private TabPage BuildRawTab()
        {
            TabPage page = new TabPage("原始报文") { BackColor = BackColor, Padding = new Padding(8) };
            TableLayoutPanel t = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 2 }; t.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50)); t.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50)); t.RowStyles.Add(new RowStyle(SizeType.Percent, 100)); t.RowStyles.Add(new RowStyle(SizeType.Absolute, 48)); page.Controls.Add(t);
            txtRawTx = HexBox("01 04 00 00 00 06"); txtRawRx = HexBox(""); txtRawRx.ReadOnly = true;
            t.Controls.Add(Group("发送十六进制", txtRawTx), 0, 0); t.Controls.Add(Group("接收十六进制", txtRawRx), 1, 0);
            FlowLayoutPanel bar = new FlowLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(5, 7, 0, 0) };
            chkAppendCrc = new CheckBox { Text = "自动追加 Modbus CRC", Checked = true, AutoSize = true, Padding = new Padding(0, 5, 0, 0) }; bar.Controls.Add(chkAppendCrc);
            btnRawCrc = ButtonOf("计算 CRC", 78); btnRawCrc.Click += delegate { CalculateRawCrc(); }; bar.Controls.Add(btnRawCrc);
            btnRawSend = AccentButton("发送并接收", 96); btnRawSend.Click += SendRaw; bar.Controls.Add(btnRawSend); t.SetColumnSpan(bar, 2); t.Controls.Add(bar, 0, 1); return page;
        }

        private TabPage BuildLegacyTab()
        {
            TabPage page = new TabPage("初赛协议与升级") { BackColor = BackColor, Padding = new Padding(8) };
            TableLayoutPanel t = new TableLayoutPanel { Dock = DockStyle.Fill, RowCount = 2, ColumnCount = 1 }; t.RowStyles.Add(new RowStyle(SizeType.Percent, 45)); t.RowStyles.Add(new RowStyle(SizeType.Percent, 55)); page.Controls.Add(t);
            Panel framePanel = CardPanel(); TableLayoutPanel ft = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(10), RowCount = 3, ColumnCount = 6 };
            ft.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); ft.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 92)); ft.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); ft.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 92)); ft.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); ft.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90)); framePanel.Controls.Add(ft);
            ft.Controls.Add(Caption("设备 ID"), 0, 0); numLegacyId = Number(1, 65534, 1, 80); numLegacyId.Hexadecimal = true; ft.Controls.Add(numLegacyId, 1, 0);
            ft.Controls.Add(Caption("命令字"), 2, 0); numLegacyCmd = Number(0, 65535, 0x0104, 80); numLegacyCmd.Hexadecimal = true; ft.Controls.Add(numLegacyCmd, 3, 0);
            ft.Controls.Add(Caption("Payload HEX"), 0, 1); txtLegacyPayload = new TextBox { Dock = DockStyle.Fill }; ft.SetColumnSpan(txtLegacyPayload, 4); ft.Controls.Add(txtLegacyPayload, 1, 1);
            btnLegacyBuild = ButtonOf("生成", 72); btnLegacyBuild.Click += delegate { BuildLegacyFrame(); }; ft.Controls.Add(btnLegacyBuild, 5, 0);
            btnLegacySend = AccentButton("发送", 72); btnLegacySend.Click += SendLegacy; ft.Controls.Add(btnLegacySend, 5, 1);
            txtLegacyBuilt = new TextBox { Dock = DockStyle.Fill, ReadOnly = true, Font = new Font("Consolas", 9F), Multiline = true, ScrollBars = ScrollBars.Vertical }; ft.SetColumnSpan(txtLegacyBuilt, 6); ft.Controls.Add(txtLegacyBuilt, 0, 2); t.Controls.Add(framePanel, 0, 0);
            Panel up = CardPanel(); TableLayoutPanel ut = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(10), RowCount = 5, ColumnCount = 4 }; ut.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize)); ut.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); ut.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 80)); ut.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 105)); up.Controls.Add(ut);
            Label warn = new Label { Text = "固件升级会擦除 APP 区。当前纯 Modbus APP 不处理 0x0501，必须先实现升级入口或已进入 Bootloader；接收缓冲上限 48KB。", Dock = DockStyle.Fill, ForeColor = Red, Font = new Font(UiFont, FontStyle.Bold), AutoSize = false, TextAlign = ContentAlignment.MiddleLeft }; ut.SetColumnSpan(warn, 4); ut.Controls.Add(warn, 0, 0);
            ut.Controls.Add(Caption("固件"), 0, 1); txtFirmware = new TextBox { Dock = DockStyle.Fill }; ut.Controls.Add(txtFirmware, 1, 1); btnFirmwareBrowse = ButtonOf("选择", 64); btnFirmwareBrowse.Click += SelectFirmware; ut.Controls.Add(btnFirmwareBrowse, 2, 1);
            chkUpgradeRisk = new CheckBox { Text = "我已确认恢复方式和目标分区", AutoSize = true, Padding = new Padding(0, 6, 0, 0) }; ut.SetColumnSpan(chkUpgradeRisk, 2); ut.Controls.Add(chkUpgradeRisk, 1, 2);
            btnUpgrade = new Button { Text = "执行升级", Dock = DockStyle.Fill, BackColor = Red, ForeColor = Color.White, FlatStyle = FlatStyle.Flat }; btnUpgrade.FlatAppearance.BorderSize = 0; btnUpgrade.Click += StartUpgrade; ut.SetRowSpan(btnUpgrade, 2); ut.Controls.Add(btnUpgrade, 3, 1);
            upgradeProgress = new ProgressBar { Dock = DockStyle.Fill, Minimum = 0, Maximum = 100 }; ut.SetColumnSpan(upgradeProgress, 3); ut.Controls.Add(upgradeProgress, 0, 3);
            lblUpgrade = new Label { Text = "等待", Dock = DockStyle.Fill, ForeColor = Color.DimGray }; ut.SetColumnSpan(lblUpgrade, 4); ut.Controls.Add(lblUpgrade, 0, 4); t.Controls.Add(up, 0, 1); return page;
        }

        private Control BuildLogPanel()
        {
            GroupBox g = new GroupBox { Text = "通信日志", Dock = DockStyle.Fill, Padding = new Padding(7) };
            ToolStrip tools = new ToolStrip { GripStyle = ToolStripGripStyle.Hidden, Dock = DockStyle.Top };
            ToolStripButton clear = new ToolStripButton("清空"); clear.Click += delegate { txtLog.Clear(); }; ToolStripButton save = new ToolStripButton("保存日志"); save.Click += SaveLog;
            tools.Items.Add(clear); tools.Items.Add(save); txtLog = new RichTextBox { Dock = DockStyle.Fill, ReadOnly = true, Font = new Font("Consolas", 8.5F), BackColor = Color.FromArgb(249, 250, 251), BorderStyle = BorderStyle.None, HideSelection = false };
            g.Controls.Add(txtLog); g.Controls.Add(tools); return g;
        }

        private void ToggleConnection(object sender, EventArgs e)
        {
            try
            {
                StopMonitor();
                if (transport.IsOpen) { transport.Close(); SetConnected(false); return; }
                if (simulated) ToggleSimulation(null, EventArgs.Empty);
                if (string.IsNullOrWhiteSpace(cbPort.Text)) throw new InvalidOperationException("没有选择串口。");
                transport.Open(cbPort.Text, ParseInt(cbBaud.Text, 19200), (Parity)Enum.Parse(typeof(Parity), cbParity.Text), 8, cbStop.Text == "2" ? StopBits.Two : StopBits.One);
                SetConnected(true); AddLog("INFO", "串口已打开：" + cbPort.Text + " @ " + cbBaud.Text + " " + cbParity.Text + " 8" + cbStop.Text);
            }
            catch (Exception ex) { ReportError(ex); }
        }

        private void ToggleSimulation(object sender, EventArgs e)
        {
            StopMonitor(); if (transport.IsOpen) transport.Close(); simulated = !simulated; simulationCount = 0;
            btnSimulate.Text = simulated ? "退出模拟" : "离线模拟"; btnSimulate.BackColor = simulated ? Color.FromArgb(221, 238, 230) : SystemColors.Control;
            lblConnection.Text = simulated ? "模拟设备" : "未连接"; lblConnection.ForeColor = simulated ? Green : Red; btnConnect.Enabled = !simulated;
            AddLog("INFO", simulated ? "已进入离线模拟，可演练监控与寄存器读写。" : "已退出离线模拟。");
        }

        private void SetConnected(bool connected)
        {
            btnConnect.Text = connected ? "关闭串口" : "打开串口"; lblConnection.Text = connected ? "已连接 " + transport.PortName : "未连接"; lblConnection.ForeColor = connected ? Green : Red;
            cbPort.Enabled = cbBaud.Enabled = cbParity.Enabled = cbStop.Enabled = !connected; btnSimulate.Enabled = !connected;
        }

        private void ToggleMonitor(object sender, EventArgs e)
        {
            if (monitorRunning) { StopMonitor(); return; }
            if (!simulated && !transport.IsOpen) { ReportError(new InvalidOperationException("请先打开串口或启用离线模拟。")); return; }
            monitorSlave = Slave(); monitorTimeout = (int)numTimeout.Value; monitorPeriod = (int)numPoll.Value;
            monitorRunning = true; monitorCts = new CancellationTokenSource(); btnMonitor.Text = "停止轮询"; btnMonitor.BackColor = Red; lblPollState.Text = "运行中"; lblPollState.ForeColor = Green;
            Task.Run(() => MonitorLoop(monitorCts.Token));
        }

        private void StopMonitor()
        {
            monitorRunning = false; if (monitorCts != null) monitorCts.Cancel();
            if (btnMonitor != null) { btnMonitor.Text = "开始轮询"; btnMonitor.BackColor = Blue; }
            if (lblPollState != null) { lblPollState.Text = "停止"; lblPollState.ForeColor = Color.DimGray; }
        }

        private void MonitorLoop(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                DateTime started = DateTime.Now;
                try
                {
                    ushort[] regs = simulated ? SimulatedInputRegisters() : ReadRegistersCore(monitorSlave, monitorTimeout, 4, 0, 9);
                    double current = regs[0] / 1000.0, v1 = regs[1] / 1000.0, v2 = regs[2] / 1000.0;
                    ushort status = regs[3]; uint count = ((uint)regs[4] << 16) | regs[5];
                    double adc0Mv = (short)regs[6], adc1Mv = (short)regs[7], adc2Mv = (short)regs[8];
                    BeginInvoke(new Action(() => ShowMeasurement(started, current, v1, v2, adc0Mv, adc1Mv, adc2Mv, status, count)));
                }
                catch (Exception ex) { errorFrames++; BeginInvoke(new Action(() => { AddLog("ERR", ex.Message); UpdateCounters(); })); }
                int wait = monitorPeriod - (int)(DateTime.Now - started).TotalMilliseconds; if (wait < 20) wait = 20;
                token.WaitHandle.WaitOne(wait);
            }
        }

        private ushort[] SimulatedInputRegisters()
        {
            simulationCount += 5; double x = simulationCount / 20.0;
            double current = 12.0 + 7.2 * Math.Sin(x / 4.0) + (random.NextDouble() - .5) * .02;
            double v1 = 5.0 + 4.5 * Math.Sin(x / 6.0 + 1.2); double v2 = 5.0 + 4.2 * Math.Cos(x / 7.0);
            ushort status = 2; if (current < 3.5) status |= 1;
            return new ushort[] {
                ClampU16(current * 1000), ClampU16(v1 * 1000), ClampU16(v2 * 1000),
                status, (ushort)(simulationCount >> 16), (ushort)simulationCount,
                SignedRegister(current / 11.037528 * 1000.0),
                SignedRegister(v1 / 5.5 * 1000.0),
                SignedRegister(v2 / 5.5 * 1000.0)
            };
        }

        private void ShowMeasurement(DateTime time, double current, double v1, double v2,
                                     double adc0Mv, double adc1Mv, double adc2Mv,
                                     ushort status, uint count)
        {
            MeasurementPoint point = new MeasurementPoint {
                Time = time, Current = current, Voltage1 = v1, Voltage2 = v2,
                Adc0Mv = adc0Mv, Adc1Mv = adc1Mv, Adc2Mv = adc2Mv,
                Status = status, Count = count
            };
            measurementHistory.Add(point);
            if (measurementHistory.Count > MaxHistoryPoints) measurementHistory.RemoveAt(0);
            UpdateLatestMeasurement(point);
            gridMonitor.Rows.Insert(0, time.ToString("HH:mm:ss.fff"), current.ToString("F3"), v1.ToString("F3"), v2.ToString("F3"), RawAdcText(adc0Mv), RawAdcText(adc1Mv), RawAdcText(adc2Mv), StatusText(status), count);
            while (gridMonitor.Rows.Count > 300) gridMonitor.Rows.RemoveAt(gridMonitor.Rows.Count - 1);
            if (!chkChartFrozen.Checked) AppendChartPoint(point);
            lblChartSamples.Text = "缓存 " + measurementHistory.Count + " / " + MaxHistoryPoints;
            if (chkCsv.Checked) AppendCsv(time, current, v1, v2, status, count);
        }

        private void UpdateLatestMeasurement(MeasurementPoint point)
        {
            valCurrent.Text = point.Current.ToString("F3") + " mA"; valVoltage1.Text = point.Voltage1.ToString("F3") + " V"; valVoltage2.Text = point.Voltage2.ToString("F3") + " V"; valCount.Text = point.Count.ToString(CultureInfo.InvariantCulture);
            lblAdcRaw.Text = "ADC原始: AIN0 " + RawAdcText(point.Adc0Mv) + " / AIN1 " + RawAdcText(point.Adc1Mv) + " / AIN2 " + RawAdcText(point.Adc2Mv) + " mV";
            SetBadge(badgeWire, (point.Status & 1) != 0 ? "断线" : "电流正常", (point.Status & 1) != 0 ? Red : Green);
            SetBadge(badgeTfReady, (point.Status & 2) != 0 ? "TF 就绪" : "TF 未就绪", (point.Status & 2) != 0 ? Green : Amber);
            SetBadge(badgeTfError, (point.Status & 4) != 0 ? "存储错误" : "存储正常", (point.Status & 4) != 0 ? Red : Green);
        }

        private int VisibleChartPoints()
        {
            int value; return cbChartPoints != null && int.TryParse(cbChartPoints.Text, out value) ? value : 300;
        }

        private void AppendChartPoint(MeasurementPoint point)
        {
            if (monitorChart == null) return;
            monitorChart.Series["电流"].Points.AddXY(point.Time, point.Current);
            monitorChart.Series["电压 1"].Points.AddXY(point.Time, point.Voltage1);
            monitorChart.Series["电压 2"].Points.AddXY(point.Time, point.Voltage2);
            int limit = VisibleChartPoints();
            foreach (Series series in monitorChart.Series)
                while (series.Points.Count > limit) series.Points.RemoveAt(0);
        }

        private void RebuildChart()
        {
            if (monitorChart == null) return;
            foreach (Series series in monitorChart.Series) series.Points.Clear();
            int start = Math.Max(0, measurementHistory.Count - VisibleChartPoints());
            for (int i = start; i < measurementHistory.Count; i++)
            {
                MeasurementPoint point = measurementHistory[i];
                monitorChart.Series["电流"].Points.AddXY(point.Time, point.Current);
                monitorChart.Series["电压 1"].Points.AddXY(point.Time, point.Voltage1);
                monitorChart.Series["电压 2"].Points.AddXY(point.Time, point.Voltage2);
            }
            monitorChart.Invalidate();
        }

        private void ClearMonitorData()
        {
            measurementHistory.Clear(); gridMonitor.Rows.Clear();
            if (monitorChart != null) foreach (Series series in monitorChart.Series) series.Points.Clear();
            if (lblChartSamples != null) lblChartSamples.Text = "缓存 0 / " + MaxHistoryPoints;
            AddLog("INFO", "趋势缓存与数据表已清空；串口接收和 CSV 记录状态未改变。");
        }

        private void ImportMonitorCsv(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog { Filter = "监控 CSV|*.csv|所有文件|*.*", Title = "导入监控记录" };
            if (dialog.ShowDialog() != DialogResult.OK) return;
            try { StopMonitor(); LoadMonitorCsv(dialog.FileName); }
            catch (Exception ex) { ReportError(new InvalidDataException("CSV 导入失败：" + ex.Message, ex)); }
        }

        private void LoadMonitorCsv(string path)
        {
            string[] lines = File.ReadAllLines(path, Encoding.UTF8);
            List<MeasurementPoint> loaded = new List<MeasurementPoint>();
            int skipped = 0;
            for (int i = 0; i < lines.Length; i++)
            {
                if (string.IsNullOrWhiteSpace(lines[i]) || lines[i].StartsWith("pc_time", StringComparison.OrdinalIgnoreCase)) continue;
                string[] parts = lines[i].Split(',');
                DateTime time; double current, v1, v2; ushort status; uint count;
                if (parts.Length < 6 || !DateTime.TryParse(parts[0].Trim(), CultureInfo.InvariantCulture, DateTimeStyles.AllowWhiteSpaces, out time)
                    || !double.TryParse(parts[1].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out current)
                    || !double.TryParse(parts[2].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out v1)
                    || !double.TryParse(parts[3].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out v2)
                    || !TryParseStatus(parts[4], out status)
                    || !uint.TryParse(parts[5].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out count))
                { skipped++; continue; }
                loaded.Add(new MeasurementPoint { Time = time, Current = current, Voltage1 = v1, Voltage2 = v2, Adc0Mv = double.NaN, Adc1Mv = double.NaN, Adc2Mv = double.NaN, Status = status, Count = count });
            }
            if (loaded.Count == 0) throw new InvalidDataException("没有找到有效记录。需要列：pc_time,current_mA,voltage1_V,voltage2_V,status,sample_count。");
            measurementHistory.Clear();
            int first = Math.Max(0, loaded.Count - MaxHistoryPoints);
            for (int i = first; i < loaded.Count; i++) measurementHistory.Add(loaded[i]);
            gridMonitor.Rows.Clear();
            for (int i = measurementHistory.Count - 1; i >= Math.Max(0, measurementHistory.Count - 300); i--)
            {
                MeasurementPoint point = measurementHistory[i];
                gridMonitor.Rows.Add(point.Time.ToString("HH:mm:ss.fff"), point.Current.ToString("F3"), point.Voltage1.ToString("F3"), point.Voltage2.ToString("F3"), RawAdcText(point.Adc0Mv), RawAdcText(point.Adc1Mv), RawAdcText(point.Adc2Mv), StatusText(point.Status), point.Count);
            }
            UpdateLatestMeasurement(measurementHistory[measurementHistory.Count - 1]);
            lblChartSamples.Text = "缓存 " + measurementHistory.Count + " / " + MaxHistoryPoints;
            RebuildChart();
            AddLog("INFO", "已导入 CSV：" + measurementHistory.Count + " 条" + (skipped > 0 ? "，跳过 " + skipped + " 条无效记录" : "") + "。");
        }

        private static bool TryParseStatus(string text, out ushort status)
        {
            string value = text.Trim();
            return value.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                ? ushort.TryParse(value.Substring(2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out status)
                : ushort.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out status);
        }

        private void ExportChartImage(object sender, EventArgs e)
        {
            if (monitorChart == null || monitorChart.Series["电流"].Points.Count == 0) { MessageBox.Show("当前没有可导出的曲线数据。", "导出曲线图"); return; }
            SaveFileDialog dialog = new SaveFileDialog { Filter = "PNG 图片|*.png", FileName = "trend_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".png" };
            if (dialog.ShowDialog() != DialogResult.OK) return;
            try { monitorChart.SaveImage(dialog.FileName, ChartImageFormat.Png); AddLog("INFO", "曲线图已导出：" + dialog.FileName); }
            catch (Exception ex) { ReportError(ex); }
        }

        private ushort[] ReadRegistersCore(byte slave, int timeout, byte function, ushort start, ushort count)
        {
            byte[] req = ModbusRtu.ReadRegisters(slave, function, start, count); byte[] raw = transport.TransactModbus(req, timeout); ModbusResponse r = ModbusRtu.Parse(raw, slave, function);
            if (r.IsException) throw new InvalidOperationException("Modbus 异常：" + ModbusExceptionText(r.ExceptionCode.Value)); return r.Registers;
        }

        private void ReadGeneric(object sender, EventArgs e)
        {
            byte fn = byte.Parse(cbFunction.Text, NumberStyles.HexNumber);
            if (fn != 3 && fn != 4) fn = cbRegType.SelectedIndex == 0 ? (byte)4 : (byte)3;
            ushort start = (ushort)numStart.Value, count = (ushort)numCount.Value;
            byte slave = Slave(); int timeout = (int)numTimeout.Value; bool useSimulation = simulated;
            RunOperation(delegate
            {
                ushort[] regs = useSimulation ? SimulatedGeneric(start, count) : ReadRegistersCore(slave, timeout, fn, start, count);
                BeginInvoke(new Action(() => FillRegisterGrid(regs, start, fn)));
            });
        }

        private void WriteGeneric(object sender, EventArgs e)
        {
            string writeText = txtWriteValues.Text; ushort address = (ushort)numStart.Value;
            byte slave = Slave(); int timeout = (int)numTimeout.Value; bool forceMultiple = cbFunction.Text == "10"; bool useSimulation = simulated;
            RunOperation(delegate
            {
                ushort[] values = ParseUshortValues(writeText); if (values.Length == 0) throw new InvalidOperationException("请输入写入值。");
                if (useSimulation) { Thread.Sleep(80); BeginInvoke(new Action(() => AddLog("INFO", "模拟写入完成：" + string.Join(",", Array.ConvertAll(values, x => x.ToString()))))); return; }
                byte fn = values.Length == 1 && !forceMultiple ? (byte)6 : (byte)16;
                byte[] req = fn == 6 ? ModbusRtu.WriteSingle(slave, address, values[0]) : ModbusRtu.WriteMultiple(slave, address, values);
                ModbusResponse r = ModbusRtu.Parse(transport.TransactModbus(req, timeout), slave, fn);
                if (r.IsException) throw new InvalidOperationException("Modbus 异常：" + ModbusExceptionText(r.ExceptionCode.Value));
                BeginInvoke(new Action(() => AddLog("INFO", "寄存器写入成功。注意：当前固件 40001~40003 写入后不会改变调度周期。")));
            });
        }

        private void ScanSlaves(object sender, EventArgs e)
        {
            bool useSimulation = simulated; int scanTimeout = (int)numTimeout.Value;
            if (useSimulation) { MessageBox.Show("模拟设备地址：1", "扫描结果"); return; }
            RunOperation(delegate
            {
                List<int> found = new List<int>();
                for (int address = 1; address <= 247; address++)
                {
                    BeginInvoke(new Action(() => SetStatus("扫描从站 " + address + "/247")));
                    try { byte[] req = ModbusRtu.ReadRegisters((byte)address, 4, 0, 1); ModbusRtu.Parse(transport.TransactModbus(req, Math.Min(scanTimeout, 120)), (byte)address, 4); found.Add(address); } catch { }
                }
                BeginInvoke(new Action(() => MessageBox.Show(found.Count == 0 ? "未发现设备。" : "发现地址：" + string.Join(", ", found), "扫描结果")));
            });
        }

        private void SendRaw(object sender, EventArgs e)
        {
            string rawText = txtRawTx.Text; bool appendCrc = chkAppendCrc.Checked; int timeout = (int)numTimeout.Value; bool useSimulation = simulated;
            RunOperation(delegate
            {
                if (useSimulation) throw new InvalidOperationException("原始报文不支持离线模拟。");
                byte[] raw = ByteUtil.ParseHex(rawText); if (appendCrc) raw = AppendCrc(raw);
                transport.DiscardInput(); transport.WriteRaw(raw, true); byte[] rx = transport.ReadUntilIdle(timeout, 30);
                BeginInvoke(new Action(() => txtRawRx.Text = ByteUtil.ToHex(rx)));
            });
        }

        private void CalculateRawCrc()
        {
            try { byte[] raw = ByteUtil.ParseHex(txtRawTx.Text); ushort crc = ByteUtil.Crc16Modbus(raw, 0, raw.Length); MessageBox.Show("CRC = 0x" + crc.ToString("X4") + "\n发送顺序：" + (byte)crc + " " + (byte)(crc >> 8) + "（低字节在前）", "CRC16-Modbus"); } catch (Exception ex) { ReportError(ex); }
        }

        private void BuildLegacyFrame()
        {
            try { byte[] frame = InitialProtocol.BuildAscii((ushort)numLegacyId.Value, 1, (ushort)numLegacyCmd.Value, ByteUtil.ParseHex(txtLegacyPayload.Text)); txtLegacyBuilt.Text = Encoding.ASCII.GetString(frame); } catch (Exception ex) { ReportError(ex); }
        }

        private void SendLegacy(object sender, EventArgs e)
        {
            ushort id = (ushort)numLegacyId.Value, command = (ushort)numLegacyCmd.Value;
            string payloadText = txtLegacyPayload.Text; int timeout = (int)numTimeout.Value; bool useSimulation = simulated;
            RunOperation(delegate
            {
                if (useSimulation) throw new InvalidOperationException("初赛协议不支持离线模拟。");
                byte[] frame = InitialProtocol.BuildAscii(id, 1, command, ByteUtil.ParseHex(payloadText));
                byte[] rx = transport.TransactAscii(frame, timeout);
                BeginInvoke(new Action(() => txtLegacyBuilt.Text = "TX: " + Encoding.ASCII.GetString(frame) + Environment.NewLine + "RX: " + Encoding.ASCII.GetString(rx)));
            });
        }

        private void StartUpgrade(object sender, EventArgs e)
        {
            if (!chkUpgradeRisk.Checked) { MessageBox.Show("请先确认恢复方式和目标分区。", "升级保护", MessageBoxButtons.OK, MessageBoxIcon.Warning); return; }
            if (!transport.IsOpen) { ReportError(new InvalidOperationException("请先用 Bootloader 当前波特率打开串口。")); return; }
            string path = txtFirmware.Text; if (!File.Exists(path)) { ReportError(new FileNotFoundException("固件文件不存在。")); return; }
            byte[] fw = File.ReadAllBytes(path);
            if (!InitialProtocol.HasFirmwareMagic(fw))
            {
                if (MessageBox.Show("所选 bin 没有升级魔术字。是否自动在文件数据前补入 5A A5 C3 3C？\n原文件不会被修改。", "准备升级固件", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;
                fw = InitialProtocol.PrepareFirmware(fw);
                AddLog("INFO", "已在内存中自动补入升级魔术字，原 bin 文件未修改。");
            }
            if (fw.Length > 48 * 1024) { ReportError(new InvalidOperationException("固件超过当前 Bootloader 48KB 接收缓冲。")); return; }
            if (MessageBox.Show("将擦除并重写 APP 区，继续吗？", "确认固件升级", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes) return;
            ushort upgradeId = (ushort)numLegacyId.Value; int upgradeTimeout = Math.Max((int)numTimeout.Value, 2500); StopMonitor(); RunOperation(delegate { UpgradeCore(fw, upgradeId, upgradeTimeout); });
        }

        private void UpgradeCore(byte[] firmware, ushort id, int timeout)
        {
            SetUpgrade(1, "步骤 1/3：通知 APP 进入 Bootloader（若已在 Bootloader，可忽略超时）");
            try { byte[] req = InitialProtocol.BuildAscii(id, 1, InitialProtocol.UpgradeRequest, null); transport.TransactAscii(req, timeout); } catch (Exception ex) { AddLogThreadSafe("WARN", "升级请求未应答：" + ex.Message); }
            Thread.Sleep(1200); transport.DiscardInput();
            SetUpgrade(5, "步骤 2/3：发送准备命令"); byte[] prepare = InitialProtocol.BuildAscii(id, 1, InitialProtocol.UpgradePrepare, null); transport.WriteRaw(prepare, true);
            Thread.Sleep(500);
            int sent = 0; while (sent < firmware.Length)
            {
                int n = Math.Min(256, firmware.Length - sent); byte[] block = new byte[n]; Buffer.BlockCopy(firmware, sent, block, 0, n); transport.WriteRaw(block, false); sent += n;
                SetUpgrade(5 + (int)(80L * sent / firmware.Length), "发送固件 " + sent + "/" + firmware.Length + " 字节"); Thread.Sleep(5);
            }
            byte[] prepareAck = transport.ReadUntilIdle(2500, 80); if (!InitialProtocol.IsAckOk(prepareAck, InitialProtocol.UpgradePrepare)) throw new InvalidOperationException("Bootloader 未确认固件接收，请勿发送执行命令。");
            SetUpgrade(90, "步骤 3/3：发送执行命令"); byte[] exec = InitialProtocol.BuildAscii(id, 1, InitialProtocol.UpgradeExecute, null); byte[] execAck = transport.TransactAscii(exec, timeout);
            if (!InitialProtocol.IsAckOk(execAck, InitialProtocol.UpgradeExecute)) throw new InvalidOperationException("Bootloader 未确认执行命令。");
            SetUpgrade(100, "升级命令完成，设备将复位；请重新连接并验证版本。等待 5 秒后方可断电。"); AddLogThreadSafe("INFO", "固件升级流程完成。必须重新连接并验证 APP 版本/寄存器。 ");
        }

        private void RunOperation(Action action)
        {
            lock (operationGate) { if (operationBusy) { SetStatus("已有串口任务运行中"); return; } operationBusy = true; }
            SetStatus("执行中..."); Task.Run(delegate
            {
                try { action(); BeginInvoke(new Action(() => SetStatus("完成"))); }
                catch (Exception ex) { errorFrames++; BeginInvoke(new Action(() => ReportError(ex))); }
                finally { lock (operationGate) operationBusy = false; BeginInvoke(new Action(UpdateCounters)); }
            });
        }

        private void OnTrace(string direction, byte[] data)
        {
            if (direction == "TX") txFrames++; else rxFrames++;
            AddLogThreadSafe(direction, ByteUtil.ToHex(data)); BeginInvoke(new Action(UpdateCounters));
        }

        private void AddLogThreadSafe(string kind, string message)
        {
            if (IsDisposed) return; if (InvokeRequired) { BeginInvoke(new Action(() => AddLog(kind, message))); return; } AddLog(kind, message);
        }

        private void AddLog(string kind, string message)
        {
            Color c = kind == "ERR" ? Red : kind == "TX" ? Blue : kind == "RX" ? Green : kind == "WARN" ? Amber : Ink;
            txtLog.SelectionStart = txtLog.TextLength; txtLog.SelectionColor = Color.Gray; txtLog.AppendText(DateTime.Now.ToString("HH:mm:ss.fff") + " ");
            txtLog.SelectionColor = c; txtLog.AppendText("[" + kind + "] "); txtLog.SelectionColor = Ink; txtLog.AppendText(message + Environment.NewLine); txtLog.ScrollToCaret();
        }

        private void ReportError(Exception ex) { AddLog("ERR", ex.Message); SetStatus("失败：" + ex.Message); UpdateCounters(); }
        private void SetStatus(string text) { statusText.Text = text; }
        private void UpdateCounters() { statusCounters.Text = "TX " + txFrames + "  RX " + rxFrames + "  ERR " + errorFrames; }
        private void SetUpgrade(int progress, string text) { BeginInvoke(new Action(() => { upgradeProgress.Value = Math.Max(0, Math.Min(100, progress)); lblUpgrade.Text = text; })); }

        private void FillRegisterGrid(ushort[] regs, ushort start, byte fn)
        {
            gridRegisters.Rows.Clear(); string prefix = fn == 4 ? "3" : "4";
            for (int i = 0; i < regs.Length; i++) gridRegisters.Rows.Add(start + i, prefix + (start + i + 1).ToString("D4"), regs[i], unchecked((short)regs[i]), "0x" + regs[i].ToString("X4"));
        }

        private void SyncRegType() { cbFunction.Text = cbRegType.SelectedIndex == 0 ? "04" : "03"; }
        private ushort[] SimulatedGeneric(ushort start, ushort count) { ushort[] r = new ushort[count]; ushort[] input = SimulatedInputRegisters(); for (int i = 0; i < count; i++) r[i] = start + i < input.Length ? input[start + i] : (ushort)0; return r; }
        private static ushort ClampU16(double value) { return (ushort)Math.Max(0, Math.Min(65535, Math.Round(value))); }
        private static ushort SignedRegister(double value) { int x = (int)Math.Max(short.MinValue, Math.Min(short.MaxValue, Math.Round(value))); return unchecked((ushort)(short)x); }
        private static string RawAdcText(double value) { return double.IsNaN(value) ? "--" : value.ToString("F0", CultureInfo.InvariantCulture); }
        private byte Slave() { int s; if (!int.TryParse(cbSlave.Text, out s) || s < 1 || s > 247) throw new InvalidOperationException("从站地址必须为 1~247。"); return (byte)s; }
        private static int ParseInt(string text, int fallback) { int x; return int.TryParse(text, out x) ? x : fallback; }
        private static string ModbusExceptionText(byte code) { return code == 1 ? "01 非法功能" : code == 2 ? "02 非法地址" : code == 3 ? "03 非法数值" : code == 4 ? "04 设备故障" : "0x" + code.ToString("X2"); }
        private static string StatusText(ushort s) { List<string> x = new List<string>(); if ((s & 1) != 0) x.Add("断线"); if ((s & 2) != 0) x.Add("TF就绪"); if ((s & 4) != 0) x.Add("TF错误"); return x.Count == 0 ? "正常" : string.Join("/", x); }

        private static ushort[] ParseUshortValues(string text)
        {
            string[] parts = text.Split(new char[] { ' ', ',', ';', '\r', '\n', '\t' }, StringSplitOptions.RemoveEmptyEntries); List<ushort> values = new List<ushort>();
            foreach (string p in parts) { int value = p.StartsWith("0x", StringComparison.OrdinalIgnoreCase) ? int.Parse(p.Substring(2), NumberStyles.HexNumber) : int.Parse(p, CultureInfo.InvariantCulture); if (value < 0 || value > 65535) throw new FormatException("写入值超出 0~65535。"); values.Add((ushort)value); } return values.ToArray();
        }

        private static byte[] AppendCrc(byte[] raw) { ushort crc = ByteUtil.Crc16Modbus(raw, 0, raw.Length); byte[] r = new byte[raw.Length + 2]; Buffer.BlockCopy(raw, 0, r, 0, raw.Length); r[r.Length - 2] = (byte)crc; r[r.Length - 1] = (byte)(crc >> 8); return r; }

        private void AppendCsv(DateTime time, double current, double v1, double v2, ushort status, uint count)
        {
            try
            {
                string path = txtCsvPath.Text; string dir = Path.GetDirectoryName(path); if (!string.IsNullOrEmpty(dir)) Directory.CreateDirectory(dir); bool header = !File.Exists(path) || new FileInfo(path).Length == 0;
                using (StreamWriter sw = new StreamWriter(path, true, new UTF8Encoding(true))) { if (header) sw.WriteLine("pc_time,current_mA,voltage1_V,voltage2_V,status,sample_count"); sw.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0:yyyy-MM-dd HH:mm:ss.fff},{1:F3},{2:F3},{3:F3},0x{4:X4},{5}", time, current, v1, v2, status, count)); }
            }
            catch (Exception ex) { chkCsv.Checked = false; AddLog("ERR", "CSV 保存失败：" + ex.Message); }
        }

        private void RefreshPorts()
        {
            string old = cbPort == null ? "" : cbPort.Text; string[] ports = SerialPort.GetPortNames(); Array.Sort(ports, delegate(string a, string b) { return NaturalPort(a).CompareTo(NaturalPort(b)); }); cbPort.Items.Clear(); cbPort.Items.AddRange(ports);
            if (Array.IndexOf(ports, old) >= 0) cbPort.Text = old; else if (ports.Length > 0) cbPort.SelectedIndex = 0; SetStatus("发现 " + ports.Length + " 个串口");
        }
        private static int NaturalPort(string s) { int n; return int.TryParse(s.Replace("COM", ""), out n) ? n : 9999; }

        private void SelectCsv(object sender, EventArgs e) { SaveFileDialog d = new SaveFileDialog { Filter = "CSV 文件|*.csv|所有文件|*.*", FileName = "monitor.csv" }; if (d.ShowDialog() == DialogResult.OK) txtCsvPath.Text = d.FileName; }
        private void SelectFirmware(object sender, EventArgs e) { OpenFileDialog d = new OpenFileDialog { Filter = "固件 BIN|*.bin|所有文件|*.*" }; if (d.ShowDialog() == DialogResult.OK) { txtFirmware.Text = d.FileName; lblUpgrade.Text = new FileInfo(d.FileName).Length + " 字节"; } }
        private void SaveLog(object sender, EventArgs e) { SaveFileDialog d = new SaveFileDialog { Filter = "日志文件|*.log|文本文件|*.txt", FileName = "serial_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".log" }; if (d.ShowDialog() == DialogResult.OK) File.WriteAllText(d.FileName, txtLog.Text, new UTF8Encoding(true)); }

        public async void StartSmokeTest(string screenshotPath, string csvPath)
        {
            try
            {
                txtCsvPath.Text = csvPath; chkCsv.Checked = true; numPoll.Value = 100;
                ToggleSimulation(null, EventArgs.Empty); ToggleMonitor(null, EventArgs.Empty);
                await Task.Delay(1300); StopMonitor(); await Task.Delay(150);
                ClearMonitorData(); LoadMonitorCsv(csvPath);
                if (measurementHistory.Count < 5 || monitorChart.Series["电流"].Points.Count < 5) throw new InvalidOperationException("CSV 回放未恢复足够的曲线点。");
                monitorChart.SaveImage(Path.ChangeExtension(screenshotPath, ".chart.png"), ChartImageFormat.Png);
                using (Bitmap image = new Bitmap(ClientSize.Width, ClientSize.Height))
                {
                    DrawToBitmap(image, new Rectangle(Point.Empty, ClientSize));
                    image.Save(screenshotPath, System.Drawing.Imaging.ImageFormat.Png);
                }
                AddLog("INFO", "Smoke test passed.");
                await Task.Delay(100); Close();
            }
            catch (Exception ex)
            {
                File.WriteAllText(screenshotPath + ".error.txt", ex.ToString());
                Close();
            }
        }

        private void OnFormClosing(object sender, FormClosingEventArgs e) { StopMonitor(); transport.Dispose(); }

        private static Panel CardPanel() { return new Panel { Dock = DockStyle.Fill, BackColor = Panel, BorderStyle = BorderStyle.FixedSingle, Margin = new Padding(3) }; }
        private static Label Caption(string text) { return new Label { Text = text, AutoSize = true, ForeColor = Color.FromArgb(75, 84, 94), Padding = new Padding(4, 7, 2, 0), Margin = new Padding(1) }; }
        private static ComboBox Combo(int width) { return new ComboBox { Width = width, DropDownStyle = ComboBoxStyle.DropDown, Margin = new Padding(2, 3, 4, 2) }; }
        private static NumericUpDown Number(decimal min, decimal max, decimal value, int width) { return new NumericUpDown { Minimum = min, Maximum = max, Value = value, Width = width, Margin = new Padding(2, 3, 4, 2) }; }
        private static Button ButtonOf(string text, int width) { return new Button { Text = text, Width = width, Height = 28, FlatStyle = FlatStyle.Flat, BackColor = Color.White, ForeColor = Ink, Margin = new Padding(3, 2, 3, 2) }; }
        private static Button AccentButton(string text, int width) { Button b = ButtonOf(text, width); b.BackColor = Blue; b.ForeColor = Color.White; b.FlatAppearance.BorderSize = 0; return b; }
        private static DataGridView NewGrid() { DataGridView g = new DataGridView { Dock = DockStyle.Fill, BackgroundColor = Color.White, BorderStyle = BorderStyle.FixedSingle, AllowUserToAddRows = false, AllowUserToDeleteRows = false, ReadOnly = true, RowHeadersVisible = false, AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill, SelectionMode = DataGridViewSelectionMode.FullRowSelect }; g.EnableHeadersVisualStyles = false; g.ColumnHeadersDefaultCellStyle.BackColor = Color.FromArgb(232, 237, 241); g.ColumnHeadersDefaultCellStyle.ForeColor = Ink; return g; }
        private static TextBox HexBox(string text) { return new TextBox { Dock = DockStyle.Fill, Multiline = true, ScrollBars = ScrollBars.Both, WordWrap = true, Text = text, Font = new Font("Consolas", 11F), BackColor = Color.White }; }
        private static GroupBox Group(string title, Control child) { GroupBox g = new GroupBox { Text = title, Dock = DockStyle.Fill, Padding = new Padding(8) }; g.Controls.Add(child); return g; }
        private static Label ValueCard(FlowLayoutPanel parent, string title, string value, Color color) { Panel p = new Panel { Width = 174, Height = 83, BackColor = Color.White, BorderStyle = BorderStyle.FixedSingle, Margin = new Padding(3, 4, 5, 3) }; Label t = new Label { Text = title, Location = new Point(12, 10), AutoSize = true, ForeColor = Color.DimGray }; Label v = new Label { Text = value, Location = new Point(12, 34), AutoSize = true, Font = new Font("Microsoft YaHei UI", 17F, FontStyle.Bold), ForeColor = color }; p.Controls.Add(t); p.Controls.Add(v); parent.Controls.Add(p); return v; }
        private static Label Badge(FlowLayoutPanel parent, string text) { Label b = new Label { Text = text, Width = 88, Height = 32, TextAlign = ContentAlignment.MiddleCenter, BackColor = Color.FromArgb(235, 238, 240), ForeColor = Color.DimGray, Margin = new Padding(5, 28, 2, 2), BorderStyle = BorderStyle.FixedSingle }; parent.Controls.Add(b); return b; }
        private static void SetBadge(Label badge, string text, Color color) { badge.Text = text; badge.ForeColor = color; badge.BackColor = Color.FromArgb(248, 250, 251); }
    }
}
