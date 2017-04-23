using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using ZeroMQ;

namespace Commander
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        ZContext context;
        ZSocket publisher;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            context = new ZContext();
            publisher = new ZSocket(context, ZSocketType.PUB);
            publisher.Bind("tcp://127.0.0.1:7889");
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            context.Shutdown();
        }

        private void btnSend_Click(object sender, RoutedEventArgs e)
        {
            string address = "To Worker";
            using (var cmdOutgoing = new ZMessage())
            {
                cmdOutgoing.Add(new ZFrame(address));
                cmdOutgoing.Add(new ZFrame(txtMessage.Text));
                publisher.SendMessage(cmdOutgoing);
            }
        }

        private void btnStartMotor_Click(object sender, RoutedEventArgs e)
        {
            string address = "To Worker";
            byte type = 0x01;
            int speed = 0;
            if (!Int32.TryParse(txtSpeed.Text, out speed))
            {
                // assign default value
                speed = 100;
                txtSpeed.Text = "100";
            }

            using (var cmdOutgoing = new ZMessage())
            {
                cmdOutgoing.Add(new ZFrame(address));
                cmdOutgoing.Add(new ZFrame(type));
                cmdOutgoing.Add(new ZFrame(speed));
                publisher.SendMessage(cmdOutgoing);
            }
        }

        private void btnStopMotor_Click(object sender, RoutedEventArgs e)
        {
            string address = "To Worker";
            byte type = 0x02;

            using (var cmdOutgoing = new ZMessage())
            {
                cmdOutgoing.Add(new ZFrame(address));
                cmdOutgoing.Add(new ZFrame(type));
                publisher.SendMessage(cmdOutgoing);
            }
        }
    }
}
