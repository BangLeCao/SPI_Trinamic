using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using ZedGraph;
using System.IO.Ports;
using System.Threading;

namespace WindowsFormsApp1
{
    public partial class Form1 : Form
    {
        string stspeed;
        string setPoint;
        string setPoint1;
        string setPoint2;
        string temp = String.Empty;

        delegate void SetTextCallback(string text);
        public Form1()
        {
            InitializeComponent();
            btnPosition.Enabled = true;
            btnAuto.Enabled = true;
            this.txtSetPoint.Text = "000";
            //temp = "A" + txtSetPoint.Text + "Z";
        }
        
        private void btnConn_Click(object sender, EventArgs e)
        {
            try
            {
                if (cbPort.Text != "")
                {
                    if (cbBaud.Text != "")
                    {
                        serialPort1.PortName = cbPort.Text;
                        serialPort1.BaudRate = Convert.ToInt32(cbBaud.Text);
                        serialPort1.Parity = Parity.None;
                        serialPort1.StopBits = StopBits.One;
                        serialPort1.DataBits = 8;
                        serialPort1.Handshake = Handshake.None;
                        serialPort1.RtsEnable = true;

                        if (serialPort1.IsOpen) return;
                        serialPort1.Open();
                        btnConn.Enabled = false;
                        btnDisConn.Enabled = true;
                        btnPosition.Enabled = true;
                        btnAuto.Enabled = true;

                        cbBaud.Enabled = false;
                        cbPort.Enabled = false;

                        btnExit.Enabled = false;

                    }
                    else
                        return;
                }
                else
                    return;
            }
            catch
            {
                return;
            }
        }

        private void btnDisConn_Click(object sender, EventArgs e)
        {
            try
            {
                if (serialPort1.IsOpen == false) return;
                serialPort1.Close();
                btnConn.Enabled = true;
                btnDisConn.Enabled = false;
                btnExit.Enabled = true;

                cbBaud.Enabled = true;
                cbPort.Enabled = true;
            }
            catch
            {
                return;
            }
        }

        private void serialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                string[] arrList = serialPort1.ReadExisting().ToString().Split(',');
                if(arrList[0] == "A" && arrList[2] == "Z")
                {
                    stspeed = arrList[1];
                    SetText(stspeed);
                }
            }
            catch
            {
                return;
            }
        }
        private void SetText(string text)
        {
            if (this.txtSpeed.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText); // khởi tạo 1 delegate mới gọi đến SetText
                this.Invoke(d, new object[] { text });
            }
            else this.txtSpeed.Text = text;
        }
        int intlen = 0;

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (btnConn.Enabled == false)
            {
                Draw(stspeed, setPoint);
            }
            //auto detect COM port//
            string[] ports = SerialPort.GetPortNames();
            if (intlen != ports.Length)
            {
                intlen = ports.Length;
                cbPort.Items.Clear();
                for (int j = 0; j < intlen; j++)
                {
                    cbPort.Items.Add(ports[j]);
                }
            }
        }
        int TickStart1;
        private void Form1_Load(object sender, EventArgs e)
        {
            temp = "A" + txtSetPoint.Text + "Z";
            cbBaud.Enabled = true;
            cbPort.Enabled = true;
            btnPosition.Enabled = false;
            btnAuto.Enabled = false;
            btnStop.Enabled = false;

            cbBaud.Items.Add(9600);
            cbBaud.Items.Add(19200);
            cbBaud.Items.Add(38400);
            cbBaud.Items.Add(57600);
            cbBaud.Items.Add(74880);
            cbBaud.Items.Add(115200);

            btnDisConn.Enabled = false;

            GraphPane myPane1 = zedGraphControl1.GraphPane;
            myPane1.Title.Text = "Graph";
            myPane1.XAxis.Title.Text = "Time, Seconds";
            myPane1.YAxis.Title.Text = "Velocity, rpm";

            RollingPointPairList list1 = new RollingPointPairList(60000);
            //RollingPointPairList list2 = new RollingPointPairList(60000);

            LineItem Curve1 = myPane1.AddCurve("Result", list1, Color.Blue, SymbolType.None);
            //LineItem curve1 = myPane1.AddCurve("set_point", list2, Color.Red, SymbolType.None);

            myPane1.XAxis.Scale.Min = 0;
            myPane1.XAxis.Scale.Max = 20;
            myPane1.XAxis.Scale.MinorStep = 1;   // khoang cach giua 2 vach chia nho
            myPane1.XAxis.Scale.MajorStep = 10;
            myPane1.YAxis.Scale.Min = 0;
            myPane1.YAxis.Scale.Max = 400;

            zedGraphControl1.AxisChange();
            TickStart1 = Environment.TickCount;
        }
        private void Draw(string inspeed, string set_point)
        {
            double _speed;
            //double _setPoint;

            double.TryParse(inspeed, out _speed);
            //double.TryParse(set_point, out _setPoint);  // string to double

            if (zedGraphControl1.GraphPane.CurveList.Count <= 0)
                return;

            LineItem curve1 = zedGraphControl1.GraphPane.CurveList[0] as LineItem;
            //LineItem curve2 = zedGraphControl1.GraphPane.CurveList[1] as LineItem;

            if (curve1 == null)
                return;
            IPointListEdit list1 = curve1.Points as IPointListEdit;
            //IPointListEdit list2 = curve2.Points as IPointListEdit;

            if (list1 == null)
                return;
            double time1 = (Environment.TickCount - TickStart1) / 1000.0;
            list1.Add(time1, _speed);
            //list2.Add(time1, _setPoint);

            Scale xScale1 = zedGraphControl1.GraphPane.XAxis.Scale;
            Scale yScale1 = zedGraphControl1.GraphPane.YAxis.Scale;


            //
            if (time1 > xScale1.Max - xScale1.MajorStep)
            {
                xScale1.Max = time1 + xScale1.MajorStep;
                xScale1.Min = xScale1.Max - 50;
            }
            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();
            zedGraphControl1.Refresh();
        }

        private void btnExit_Click(object sender, EventArgs e)
        {
            serialPort1.Close();
            Close();
        }

        private void cbBaud_SelectedIndexChanged(object sender, EventArgs e)
        {
            serialPort1.Close();
            btnConn.Enabled = true;
            btnDisConn.Enabled = false;
        }
        int a;
        private void txtSetSpeed_TextChanged(object sender, EventArgs e)
        {
            //a = Int32.Parse(txtSetPoint.Text);
            btnPosition.Enabled = true;
            btnAuto.Enabled = true;
        }
        private void btnStop_Click(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                serialPort1.Write("A" + "000" + "Z");
                temp = "A" + "0" + "000" + "000" + "Z";
            }
        }

        private void btnPosition_Click(object sender, EventArgs e)
        {
            setPoint = txtSetPoint.Text;
            GraphPane myPane1 = zedGraphControl1.GraphPane;
            myPane1.XAxis.Title.Text = "Time, Seconds";
            myPane1.YAxis.Title.Text = "Position, degrees";
            myPane1.YAxis.Scale.Max = 400;
            btnStop.Enabled = true;
            if (serialPort1.IsOpen)
            {
                serialPort1.Write("A" + "0" + "000" + txtSetPoint.Text + "Z");
                temp = "A" + txtSetPoint.Text + "Z";
            }
        }

        private void txtSpeed_TextChanged(object sender, EventArgs e)
        {

        }

        private void label2_Click(object sender, EventArgs e)
        {

        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            zedGraphControl1.GraphPane.CurveList.Clear();
            zedGraphControl1.GraphPane.GraphObjList.Clear();
            zedGraphControl1.AxisChange();
            zedGraphControl1.Invalidate();
            GraphPane myPane1 = zedGraphControl1.GraphPane;
            myPane1.Title.Text = "Graph";
            myPane1.XAxis.Title.Text = "Time, Seconds";
            myPane1.YAxis.Title.Text = "Velocity, rpm";
            RollingPointPairList list1 = new RollingPointPairList(60000);
            //RollingPointPairList list2 = new RollingPointPairList(60000);

            LineItem Curve1 = myPane1.AddCurve("Result", list1, Color.Blue, SymbolType.None);
            //LineItem curve1 = myPane1.AddCurve("set_point", list2, Color.Red, SymbolType.None);



            myPane1.XAxis.Scale.Min = 0;
            myPane1.XAxis.Scale.Max = 50;
            myPane1.XAxis.Scale.MinorStep = 1;   // khoang cach giua 2 vach chia nho
            myPane1.XAxis.Scale.MajorStep = 5;
            myPane1.YAxis.Scale.Min = 0;
            myPane1.YAxis.Scale.Max = 400;

            zedGraphControl1.AxisChange();
            TickStart1 = Environment.TickCount;
        }

        private void btnAuto_Click(object sender, EventArgs e)
        {
            setPoint1 = txtPos1.Text;
            setPoint2 = txtPos2.Text;
            btnStop.Enabled = true;
            if (serialPort1.IsOpen)
            {
                serialPort1.Write("A" + "1" + txtPos1.Text + txtPos2.Text + "Z");
                temp = "A" + txtSetPoint.Text + "Z";
            }
        }

    }
}
