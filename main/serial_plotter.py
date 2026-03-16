import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets
import serial
import threading
import collections

# --- CONFIGURATION ---
COM_PORT = 'COM4' 
BAUD_RATE = 115200
WINDOW_SIZE = 200 # Number of points to display

class FastPlotter:
    def __init__(self):
        self.app = QtWidgets.QApplication([])
        self.win = pg.GraphicsLayoutWidget(title="TF-Mini High Speed Plotter")
        self.win.show()
        
        # Setup Plot
        self.plot = self.win.addPlot(title="LIDAR Distance (cm)")
        self.curve = self.plot.plot(pen='g')
        self.plot.setYRange(0, 600)
        
        # Data Buffer
        self.data = collections.deque([0]*WINDOW_SIZE, maxlen=WINDOW_SIZE)
        
        # Serial Setup
        self.ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.01)
        
        # Start background thread for reading serial (prevents UI lag)
        self.thread = threading.Thread(target=self.update_data, daemon=True)
        self.thread.start()

        # Timer to refresh the graph at 60 FPS
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(16) # ~60Hz refresh

    def update_data(self):
        while True:
            if self.ser.in_waiting:
                try:
                    # Read the line (now just a number like "123\n")
                    line = self.ser.readline().decode('utf-8').strip()
                    if line.isdigit(): # Super fast check
                        self.data.append(int(line))
                except:
                    continue

    def update_plot(self):
        self.curve.setData(list(self.data))

if __name__ == '__main__':
    p = FastPlotter()
    QtWidgets.QApplication.instance().exec()