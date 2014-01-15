import os
import sys
import wx

import serial
from serial.tools import list_ports
import struct

import matplotlib
matplotlib.use('WXAgg')
from matplotlib.figure import Figure
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as FigCanvas
import numpy as np

class GraphFrame(wx.Frame):

    title = "FabScope"

    def __init__(self):
        wx.Frame.__init__(self, None, -1, self.title)

        self.port = None
        for port, _, _ in list_ports.comports():
            try:
                port = serial.Serial(port, 115200, timeout=1)
            except:
                continue
            port.write('p')
            if port.read() == 'p':
                self.port = port
                break

        if not self.port:
            print 'error: could not find fabscope'
            sys.exit(1)

        self.data = [0]*1024
        self.paused = False

        self.create_menu()
        self.create_status_bar()
        self.create_main_panel()

        self.redraw_timer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.on_redraw_timer, self.redraw_timer)
        self.redraw_timer.Start(100)

    def create_menu(self):
        self.menubar = wx.MenuBar()

        menu_file = wx.Menu()
        m_exit = menu_file.Append(-1, "E&xit\tCtrl-X", "Exit")
        self.Bind(wx.EVT_MENU, self.on_exit, m_exit)

        self.menubar.Append(menu_file, "&File")
        self.SetMenuBar(self.menubar)

    def create_main_panel(self):
        self.panel = wx.Panel(self)

        self.init_plot()
        self.canvas = FigCanvas(self.panel, -1, self.fig)

        self.container = wx.BoxSizer(wx.VERTICAL)
        self.container.Add(self.canvas, 1, flag=wx.LEFT | wx.TOP | wx.GROW)
        self.panel.SetSizer(self.container)
        self.container.Fit(self)

    def create_status_bar(self):
        self.statusbar = self.CreateStatusBar()

    def init_plot(self):
        self.dpi = 100
        self.fig = Figure((3.0, 3.0), dpi=self.dpi)

        self.axes = self.fig.add_subplot(111)
        self.axes.set_axis_bgcolor('black')

        self.plot_data = self.axes.plot(self.data, linewidth=1, color=(1, 1, 0))[0]

    def draw_plot(self):

        self.axes.set_xbound(lower=0, upper=1023)
        self.axes.set_ybound(lower=-11.0, upper=11.0)

        self.axes.grid(True, color='gray')

        self.plot_data.set_xdata(np.arange(len(self.data)))
        self.plot_data.set_ydata(np.array(self.data))

        self.canvas.draw()

    def on_redraw_timer(self, event):
        self.port.write('r')
        raw = self.s.read(1024)
        self.data = list(struct.unpack('B'*1024, raw))
        self.data = [(float(x)/255 - 1.0) * 20 for x in self.data]
        self.draw_plot()

    def on_exit(self, event):
        self.Destroy()

    def flash_status_message(self, msg, flash_len_ms=1500):
        self.statusbar.SetStatusText(msg)
        self.timeroff = wx.Timer(self)
        self.Bind(
            wx.EVT_TIMER,
            self.on_flash_status_off,
            self.timeroff)
        self.timeroff.Start(flash_len_ms, oneShot=True)

    def on_flash_status_off(self, event):
        self.statusbar.SetStatusText('')

if __name__ == '__main__':
    app = wx.App()
    app.frame = GraphFrame()
    app.frame.Show()
    app.MainLoop()

