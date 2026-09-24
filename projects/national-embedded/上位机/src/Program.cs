using System;
using System.Windows.Forms;

namespace CimcDebugTool
{
    internal static class Program
    {
        [STAThread]
        private static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.ThreadException += delegate(object sender, System.Threading.ThreadExceptionEventArgs e)
            {
                MessageBox.Show(e.Exception.Message, "运行错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            };
            MainForm form = new MainForm();
            if (args.Length >= 3 && args[0] == "--smoke")
                form.Shown += delegate { form.StartSmokeTest(args[1], args[2]); };
            Application.Run(form);
        }
    }
}
