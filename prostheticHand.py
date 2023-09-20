from PyQt6.QtWidgets import (QApplication, QWidget, QPushButton, QLabel,
    QGridLayout, QGroupBox, QSlider, QRadioButton, QComboBox)
from PyQt6.QtCore import Qt, QTimer, QThread, pyqtSignal
from PyQt6.QtGui import QIcon, QPixmap, QImage
from PyQt6 import QtTest
import sys
import math
import serial
import time
import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import cv2
from cvzone.HandTrackingModule import HandDetector
from cvzone.SerialModule import SerialObject
import serial.tools.list_ports
import os
import mediapipe as mp

class MplCanvas(FigureCanvas):

    def __init__(self, parent=None, width=5, height=4, dpi=100):
        fig = Figure(figsize=(width, height), dpi=dpi)
        fig.set_facecolor("#f0f0f0")
        self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(fig)
        self.setParent(parent)
        fig.tight_layout()

class MainWindow(QWidget):

    def __init__(self, *args, **kwargs):
        super(MainWindow, self).__init__(*args, **kwargs)

        self.setWindowIcon(QIcon('appIcon.png'))
        self.setWindowTitle("ProstheticHand")
        self.setContentsMargins(20, 20, 20 ,20)
        self.UI()

    def UI(self):
        # Layout ------------------------------------------------------------------------------------------------------------------------------------------------
        self.layout = QGridLayout()
        self.layout.setSpacing(20)
        self.setLayout(self.layout)

        # Title ------------------------------------------------------------------------------------------------------------------------------------------------
        #self.title = QLabel("Prosthetic Hand Control")
        #layout.addWidget(self.title, 1,1)
        #self.layout.addWidget(self.title, 0, 0, 1, 4, Qt.AlignmentFlag.AlignCenter)

        # 'Control mode' buttons -------------------------------------------------------------------------------------------------------------------------------
        control_group = QGroupBox("Prosthetic Hand - Control Mode")
        control_group.setStyleSheet("""QGroupBox {font-size: 24px}""")
        self.layout.addWidget(control_group,1,1,1,4)
        control_layout = QGridLayout()
        control_group.setLayout(control_layout)
        sliderControl = QPushButton("Servos")
        sliderControl.clicked.connect(self.manualControl)
        sliderControl.setStyleSheet("""QPushButton:hover{font-size: 22px; background-color: "lightblue"}""")
        self.manualOn = False
        control_layout.addWidget(sliderControl,0,0)
        gestureControl = QPushButton("Gestures")
        gestureControl.clicked.connect(self.gestureControl)
        gestureControl.setStyleSheet("""QPushButton:hover{font-size: 22px; background-color: "lightblue"}""")
        self.gestureOn = False
        control_layout.addWidget(gestureControl,0,1)
        emgControl = QPushButton("EMG")
        self.timeCount = 0
        emgControl.clicked.connect(self.emgControl)
        emgControl.setStyleSheet("""QPushButton:hover{font-size: 22px; background-color: "lightblue"}""")
        self.emgOn = False
        control_layout.addWidget(emgControl,0,2)
        mocapControl = QPushButton("Motion Capture")
        mocapControl.clicked.connect(self.mocapControl)
        mocapControl.setStyleSheet("""QPushButton:hover{font-size: 22px; background-color: "lightblue"}""")
        self.mocapOn = False
        control_layout.addWidget(mocapControl,0,3)

        # Arduino------------------------------------------------------------------------------------------------------------------------------------------------
        arduino_group = QGroupBox("Arduino")
        arduino_group.setStyleSheet("""QGroupBox {font-size: 24px}""")
        self.layout.addWidget(arduino_group,1,0)
        arduino_layout = QGridLayout()
        arduino_group.setLayout(arduino_layout)
        board_label = QLabel("Board")
        arduino_layout.addWidget(board_label,0,0)
        port_label = QLabel("Port")
        arduino_layout.addWidget(port_label,1,0)
        baudRate_label = QLabel("Baud Rate")
        arduino_layout.addWidget(baudRate_label,2,0)
        self.arduino_board = QComboBox()
        self.arduino_board.addItems(["arduino:avr:mega"]) # would be nice to read this out from command line but need to figure out how to do this first
        arduino_layout.addWidget(self.arduino_board,0,1)
        self.arduino_baudRate = QComboBox()
        self.arduino_baudRate.addItems(["300","600","1200","2400","4800","9600","14400","19200","28800","38400","57600","76800","115200","230400","250000"])
        self.arduino_baudRate.setCurrentText('9600')
        #self.arduino_baudRate.currentTextChanged.connect(self.sendBaudRate) # Need to figure out why this leads to crazy delays --> atm changing baudRate in GUI has no effect 
        arduino_layout.addWidget(self.arduino_baudRate,2,1)
        self.arduino_portUI = QComboBox()
        # Choose port to which Arduino is connected
        available_ports = list(serial.tools.list_ports.comports())
        for p in available_ports:
            try:
                "Arduino" in p.manufacturer
                print("Found arduino")
                self.arduino_port = p.device
            except TypeError:
                continue
        self.arduino_portUI.addItems([self.arduino_port])
        #self.arduino_portUI.addItems([p.device for p in ports]) # list of all available ports
        arduino_layout.addWidget(self.arduino_portUI,1,1) 
        self.connectArduino()

        # Image ------------------------------------------------------------------------------------------------------------------------------------------------
        label = QLabel(self)
        pixmap = QPixmap('./img/inmoov_no_bg.png')
        label.setPixmap(pixmap.scaled(700, 700, Qt.AspectRatioMode.KeepAspectRatio)) # resize image (otherwise: label.setPixmap(pixmap))
        self.layout.addWidget(label, 2, 0, 6, 1, Qt.AlignmentFlag.AlignCenter)      

        # Close window -----------------------------------------------------------------------------------------------------------------------------------------
        closeButton = QPushButton("Close")
        closeButton.released.connect(self.close)
        self.layout.addWidget(closeButton,7,4,Qt.AlignmentFlag.AlignRight)

        # Initialize emg threshold -----------------------------------------------------------------------------------------------------------------------------
        self.emgThreshold = 100

    # Control states -------------------------------------------------------------------------------------------------------------------------------------------
    def manualOff(self):
        self.groupSliders.setParent(None)
        self.manualOn = False

    def gesturesOff(self):
        self.groupGestures.setParent(None)
        self.basic_label.setParent(None)
        self.advanced_label.setParent(None)
        self.rps_label.setParent(None)
        self.gestureOn = False

    def emgOff(self):
        self.timer.stop()
        self.canvas.setParent(None)
        self.emg_group.setParent(None)
        self.emgOn = False

    def mocapOff(self):
        self.FeedLabel.setParent(None)
        MainWindow.CancelFeed(self)
        self.mocapOn = False
    
    # Arduino -------------------------------------------------------------------------------------------------------------------------------------------------
    # Try to connect
    def connectArduino(self):
        try:
            self.arduino = serial.Serial(port = self.arduino_port, baudrate = self.arduino_baudRate.currentText())
            print("Connected to Arduino")
        except:
            print("Specify port manually")

        #whichSketch = "manualControl"
        #self.compileAndUpload(whichSketch) # use manual control as default

    def sendBaudRate(self):
        baudrate = int(self.arduino_baudRate.currentText())
        self.arduino.write(f'B{baudrate}'.encode())
        #time.sleep(3)
        #self.connectArduino()

    def compileAndUpload(self, whichSketch):
        arduino_cli_path = "/home/linuxbrew/.linuxbrew/bin/arduino-cli"
        arduino_board = "arduino:avr:mega"

        if whichSketch == "manualControl":
            arduino_sketch = "manualControl"
        elif whichSketch == "emgControl":
            arduino_sketch = "emgControl123"
        elif whichSketch == "mocapControl":
            arduino_sketch = "moCapCon"

        # Use arduino cli (command line interface) to compile & upload arduino sketches
        os.system(f'{arduino_cli_path} compile --fqbn {arduino_board} ./{arduino_sketch}')
        os.system(f'{arduino_cli_path} upload -p {self.arduino_port} --fqbn {arduino_board} ./{arduino_sketch}')
        print("uploaded "+arduino_sketch)
        
    # Manual Control ----------------------------------------------------------------------------------------------------------------------------------------
    def manualControl(self):
        print("Control mode: "+str(self.sender().text()))
        whichSketch = "manualControl"
        self.compileAndUpload(whichSketch)

        if self.manualOn:
            self.manualOff()
        elif self.gestureOn:
            self.gesturesOff()
        elif self.emgOn:
            self.emgOff()
        elif self.mocapOn:
            self.mocapOff()

        self.manualOn = True
        self.sliders()
    
    def sliders(self):
        self.groupSliders = QGroupBox("")
        self.layout.addWidget(self.groupSliders, 2, 1, 4, 1)
        sliderLayout = QGridLayout()
        self.groupSliders.setLayout(sliderLayout)
        # Slider labels
        self.labelThumb = QLabel("Thumb")
        sliderLayout.addWidget(self.labelThumb,1,0,Qt.AlignmentFlag.AlignCenter)
        self.labelIndex = QLabel("Index")
        sliderLayout.addWidget(self.labelIndex,1,1,Qt.AlignmentFlag.AlignCenter)
        self.labelMiddle = QLabel("Middle")
        sliderLayout.addWidget(self.labelMiddle,1,2,Qt.AlignmentFlag.AlignCenter)
        self.labelRing = QLabel("Ring")
        sliderLayout.addWidget(self.labelRing,1,3,Qt.AlignmentFlag.AlignCenter)
        self.labelPinky = QLabel("Pinky")
        sliderLayout.addWidget(self.labelPinky,1,4,Qt.AlignmentFlag.AlignCenter)
        self.labelWrist = QLabel("Wrist")
        sliderLayout.addWidget(self.labelWrist,7,0,1,5,Qt.AlignmentFlag.AlignCenter)
        # Sliders
        self.sliderThumb = QSlider(Qt.Orientation.Vertical, self)
        self.sliderThumb.setMinimum(60)
        self.sliderThumb.setMaximum(180)
        self.sliderThumb.setValue(180) 
        self.sliderThumb.valueChanged.connect(self.displayValueThumb)
        self.sliderThumb.valueChanged.connect(self.arduinoComThumb)
        self.sliderThumb.setStyleSheet(self.styleSlidersFingers())
        #slider.setGeometry(50,50, 200, 50)
        #slider.setTickPosition(QSlider.TickPosition.TicksBelow)
        #slider.setTickInterval(2)
        sliderLayout.addWidget(self.sliderThumb,3,0,Qt.AlignmentFlag.AlignHCenter)
        self.sliderIndex = QSlider(Qt.Orientation.Vertical, self)
        self.sliderIndex.setMinimum(0)
        self.sliderIndex.setMaximum(180)
        self.sliderIndex.setValue(180)
        self.sliderIndex.valueChanged.connect(self.displayValueIndex)
        self.sliderIndex.valueChanged.connect(self.arduinoComIndex)
        self.sliderIndex.setStyleSheet(self.styleSlidersFingers())
        sliderLayout.addWidget(self.sliderIndex,3,1,Qt.AlignmentFlag.AlignHCenter)
        self.sliderMiddle = QSlider(Qt.Orientation.Vertical, self)
        self.sliderMiddle.setMinimum(0)
        self.sliderMiddle.setMaximum(180)
        self.sliderMiddle.setValue(180)
        self.sliderMiddle.valueChanged.connect(self.displayValueMiddle)
        self.sliderMiddle.valueChanged.connect(self.arduinoComMiddle)
        self.sliderMiddle.setStyleSheet(self.styleSlidersFingers())
        sliderLayout.addWidget(self.sliderMiddle,3,2,Qt.AlignmentFlag.AlignHCenter)
        self.sliderRing = QSlider(Qt.Orientation.Vertical, self)
        self.sliderRing.setMinimum(0)
        self.sliderRing.setMaximum(180)
        self.sliderRing.setValue(180)
        self.sliderRing.valueChanged.connect(self.displayValueRing)
        self.sliderRing.valueChanged.connect(self.arduinoComRing)
        self.sliderRing.setStyleSheet(self.styleSlidersFingers())
        sliderLayout.addWidget(self.sliderRing,3,3,Qt.AlignmentFlag.AlignHCenter)
        self.sliderPinky = QSlider(Qt.Orientation.Vertical, self)
        self.sliderPinky.setMinimum(0)
        self.sliderPinky.setMaximum(180)
        self.sliderPinky.setValue(180)
        self.sliderPinky.valueChanged.connect(self.displayValuePinky)
        self.sliderPinky.valueChanged.connect(self.arduinoComPinky)
        self.sliderPinky.setStyleSheet(self.styleSlidersFingers())
        sliderLayout.addWidget(self.sliderPinky,3,4,Qt.AlignmentFlag.AlignHCenter)
        self.sliderWrist = QSlider(Qt.Orientation.Horizontal, self)
        self.sliderWrist.setMinimum(0)
        self.sliderWrist.setMaximum(180)
        self.sliderWrist.setValue(180)
        self.sliderWrist.valueChanged.connect(self.displayValueWrist)
        self.sliderWrist.valueChanged.connect(self.arduinoComWrist)
        self.sliderWrist.setStyleSheet(self.styleSliderWrist())
        sliderLayout.addWidget(self.sliderWrist,5,0,1,5)
        # Slider values
        self.valueThumb = QLabel(str(self.sliderThumb.value()))
        sliderLayout.addWidget(self.valueThumb,2,0,Qt.AlignmentFlag.AlignCenter)
        self.valueIndex = QLabel(str(self.sliderIndex.value()))
        sliderLayout.addWidget(self.valueIndex,2,1,Qt.AlignmentFlag.AlignCenter)
        self.valueMiddle = QLabel(str(self.sliderMiddle.value()))
        sliderLayout.addWidget(self.valueMiddle,2,2,Qt.AlignmentFlag.AlignCenter)
        self.valueRing = QLabel(str(self.sliderRing.value()))
        sliderLayout.addWidget(self.valueRing,2,3,Qt.AlignmentFlag.AlignCenter)
        self.valuePinky = QLabel(str(self.sliderPinky.value()))
        sliderLayout.addWidget(self.valuePinky,2,4,Qt.AlignmentFlag.AlignCenter)
        self.valueWrist = QLabel(str(self.sliderWrist.value()))
        sliderLayout.addWidget(self.valueWrist,6,0,1,5,Qt.AlignmentFlag.AlignCenter)

    def moveServoSliders(self, servos):
        self.sliderThumb.setValue(servos[0])
        self.sliderIndex.setValue(servos[1])
        self.sliderMiddle.setValue(servos[2])
        self.sliderRing.setValue(servos[3])
        self.sliderPinky.setValue(servos[4])
        self.sliderWrist.setValue(servos[5])
    
    def displayValueThumb(self):
        #print(self.sender().value())
        self.valueThumb.setText(str(self.sender().value())+"°")
        #self.label.adjustSize()  # Expands label size as numbers get larger
    def displayValueIndex(self):
        self.valueIndex.setText(str(self.sender().value())+"°")
    def displayValueMiddle(self):
        self.valueMiddle.setText(str(self.sender().value())+"°")
    def displayValueRing(self):
        self.valueRing.setText(str(self.sender().value())+"°")
    def displayValuePinky(self):
        self.valuePinky.setText(str(self.sender().value())+"°")
    def displayValueWrist(self):
        self.valueWrist.setText(str(self.sender().value())+"°")

    def arduinoComThumb(self):
        self.arduino.write(f'<sT{self.sender().value():03d}>'.encode())
        #print(self.arduino.readline().decode('ascii'))
    def arduinoComIndex(self):
        self.arduino.write(f'<sI{self.sender().value():03d}>'.encode())
    def arduinoComMiddle(self):
        self.arduino.write(f'<sM{self.sender().value():03d}>'.encode())
    def arduinoComRing(self):
        self.arduino.write(f'<sR{self.sender().value():03d}>'.encode())
    def arduinoComPinky(self):
        self.arduino.write(f'<sP{self.sender().value():03d}>'.encode())
    def arduinoComWrist(self):
        self.arduino.write(f'<sW{self.sender().value():03d}>'.encode())
    
    def styleSlidersFingers(self):
        return """
            QSlider::groove:vertical {
                border: 1px solid #43576a;
                height: 280px;
                background: #c5ccd2;
                margin: 0px;
                border-radius: 4px;
            }
            QSlider::handle:vertical {
                background: #dab98f;
                border: 2px solid #43576a;
                width: 8px;
                height: 24px;
                border-radius: 4px;
            }
            """
    
    def styleSliderWrist(self):
        return """
            QSlider::groove:horizontal {
                border: 1px solid #43576a;
                width: 280px;
                background: #c5ccd2;
                margin: 0px;
                border-radius: 4px;
            }
            QSlider::handle:horizontal {
                background: #ad1421;
                border: 2px solid #43576a;
                width: 24px;
                height: 8px;
                border-radius: 4px;
            }
            """

    # Gesture control --------------------------------------------------------------------------------------------------------------------------------
    def gestureControl(self):
        print("Control mode: "+str(self.sender().text()))
        whichSketch = "manualControl"
        self.compileAndUpload(whichSketch)

        if self.manualOn:
            self.manualOff()
        elif self.gestureOn:
            self.gesturesOff()
        elif self.emgOn:
            self.emgOff()
        elif self.mocapOn:
            self.mocapOff()

        self.gestureOn = True
        self.initializeGestures()

    def initializeGestures(self):
        # Layout(s)
        self.groupGestures = QGroupBox("")
        self.layout.addWidget(self.groupGestures, 3, 1, 4, 3)
        self.gesturesLayout = QGridLayout()
        self.groupGestures.setLayout(self.gesturesLayout)
        
        self.basic_label = QLabel("Basic")
        self.basic_label.setStyleSheet("""font-size: 20px; border: 10px groove #dab98f;""")
        self.layout.addWidget(self.basic_label,2,1,Qt.AlignmentFlag.AlignBottom)
        self.advanced_label = QLabel("Advanced")
        self.advanced_label.setStyleSheet("""font-size: 20px; border: 10px groove #db7d7d;""")
        self.layout.addWidget(self.advanced_label,2,2,Qt.AlignmentFlag.AlignBottom)
        self.rps_label = QLabel("Rock-Paper-Scissor")
        self.rps_label.setStyleSheet("""font-size: 20px; border: 10px groove lightblue;""")
        self.layout.addWidget(self.rps_label,2,3,Qt.AlignmentFlag.AlignBottom)

        # RadioButtons
        self.gesture1 = QRadioButton("Hand open")
        self.gesture1.setChecked(True)
        self.gesture1.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(self.gesture1,0,0)
        gesture2 = QRadioButton("Hand closed")
        gesture2.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture2,1,0)
        gesture3 = QRadioButton("Peace")
        gesture3.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture3,0,1)
        gesture4 = QRadioButton("Spiderman")
        gesture4.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture4,1,1)
        gesture5 = QRadioButton("Thumbs up")
        gesture5.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture5,2,1)
        gesture6 = QRadioButton("Point in direction")
        gesture6.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture6,3,1)
        gesture7 = QRadioButton("One")
        gesture7.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture7,2,0)
        gesture8 = QRadioButton("Two")
        gesture8.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture8,3,0)
        gesture9 = QRadioButton("Three")
        gesture9.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture9,4,0)
        gesture10 = QRadioButton("Four")
        gesture10.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture10,5,0)
        gesture11 = QRadioButton("Five")
        gesture11.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture11,6,0)
        gesture12 = QRadioButton("Bang")
        gesture12.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture12,4,1)
        gesture14 = QRadioButton("Heavy metal")
        gesture14.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture14,5,1)
        gesture15 = QRadioButton("Call me")
        gesture15.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture15,6,1)
        gesture16 = QRadioButton("Rock")
        gesture16.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture16,0,2)
        gesture17 = QRadioButton("Paper")
        gesture17.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture17,1,2)
        gesture18 = QRadioButton("Scissor")
        gesture18.pressed.connect(self.gesture)
        self.gesturesLayout.addWidget(gesture18,2,2)

    def gesture(self):
        self.arduinoComGestures()
        #self.moveServoSliders(servos) # this would move the servo sliders according to the gesture, but currently leads to an unwanted second execution of the gesture
        QtTest.QTest.qWait(5000)
        #servos = [180,180,180,180,180,180]
        self.gesture1.setChecked(True)
        #self.moveServoSliders(servos)
    
    def arduinoComGestures(self):
        self.arduino.write(f'<g{self.sender().text()}>'.encode())
        #print(self.sender().text(),":",self.sender().isChecked())
        #print(self.arduino.readline().decode('ascii'))

    # EMG --------------------------------------------------------------------------------------------------------------------------------------------
    def emgControl(self):
        print("Control mode: "+str(self.sender().text()))
        whichSketch = "emgControl"
        self.compileAndUpload(whichSketch)

        if self.manualOn:
            self.manualOff()
        elif self.gestureOn:
            self.gesturesOff()
        elif self.emgOn:
            self.emgOff()
        elif self.mocapOn:
            self.mocapOff()
        
        self.emgOn = True

        self.canvas = MplCanvas(self, width=5, height=4, dpi=100)
        #self.canvas.resize(800,600)
        self.layout.addWidget(self.canvas, 2, 1, 5, 3)

        #self.arduino = serial.Serial('/dev/ttyACM0', 9600)
        time.sleep(1)

        self.dataList = []
        self.tl = []

        self.emg_group = QGroupBox(" ")
        self.layout.addWidget(self.emg_group, 3, 4, 3, 1)
        emg_layout = QGridLayout()
        self.emg_group.setLayout(emg_layout)

        # Slider for detection threshold
        self.labelEmgThresh = QLabel("EMG Threshold")
        emg_layout.addWidget(self.labelEmgThresh,0,0,Qt.AlignmentFlag.AlignHCenter)
        self.sliderEmgThresh = QSlider(Qt.Orientation.Vertical, self)
        self.sliderEmgThresh.setMinimum(50)
        self.sliderEmgThresh.setMaximum(800)
        self.sliderEmgThresh.setValue(100)
        self.sliderEmgThresh.setStyleSheet(self.styleSliderThresh())
        self.sliderEmgThresh.valueChanged.connect(self.emgThresh)
        emg_layout.addWidget(self.sliderEmgThresh,1,0,Qt.AlignmentFlag.AlignHCenter)

        # Indicator of current control state: grip vs. rotation
        self.labelEmgControlState = QLabel("Control State")
        #emg_layout.addWidget(self.labelEmgControlState,0,2,1,2,Qt.AlignmentFlag.AlignCenter)
        gripState = QPushButton("Grip")
        gripState.setStyleSheet("""QPushButton:focus{font-size: 18px; background-color: "lightblue"}""")
        #emg_layout.addWidget(gripState,1,2)    # not added at the moment because receiving the command to indicate this from the arduino is currently slow af
        rotationState = QPushButton("Rotate")
        rotationState.setStyleSheet("""QPushButton:focus{font-size: 18px; background-color: "lightblue"}""")
        #emg_layout.addWidget(rotationState,1,3)

        # Plot
        self.update_plot()
        self.show()
        
        # Setup a timer to trigger the redraw by calling update_plot.
        self.timer = QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()

    def emgThresh(self):
        #print("EMG threshold: "+str(self.sender().value()))
        self.emgThreshold = self.sender().value()
        self.arduino.write(f't{self.emgThreshold:03d}>'.encode()) # <- uncomment to send data to Arduino. Doesn't work atm :(
        time.sleep(0.5)
        #print("EMG from Arduino"+self.arduino.readline().decode('ascii')) 

    def styleSliderThresh(self):
        return """
            QSlider::groove:vertical {
                border: 1px solid #43576a;
                height: 280px;
                background: #c5ccd2;
                margin: 0px;
                border-radius: 4px;
            }
            QSlider::handle:vertical {
                background: #43576a;
                border: 2px solid #43576a;
                width: 8px;
                height: 24px;
                border-radius: 4px;
            }
            """

    def update_plot(self):
        myStr = "r"
        self.arduino.write(myStr.encode()) # Transmit the char 'r' to receive the Arduino data point
        arduinoData_string = self.arduino.readline().decode('ascii') # Decode received Arduino data as a formatted string

        try:
            arduinoData_float = float(arduinoData_string) 
            self.dataList.append(arduinoData_float)
        except:
            pass # Pass if data point is bad
        
        # to plot time on xaxis
        self.timeCount += 1
        self.tl.append(self.timeCount)
        self.timeList = [x/10 for x in self.tl]
        if len(self.timeList) >= 50:
            self.timeList = self.timeList[-50:]

        self.ydata = self.dataList[-50:] # Drop off the first y element, append a new one.
        self.canvas.axes.cla() # Clear the canvas
        self.canvas.axes.plot(self.timeList, self.ydata, color='#db7d7d', linewidth=4) # Trigger the canvas to update and redraw.
        self.canvas.axes.axhline(self.emgThreshold, color='#43576a', linewidth=4)
        self.canvas.axes.set_xlabel("Time [s]", fontsize=18)
        self.canvas.axes.set_ylabel("EMG", fontsize=18)
        self.canvas.axes.set_facecolor(color='#e8e8e8')
        self.canvas.axes.grid(which='major', color='#CCCCCC', linestyle='--')
        self.canvas.axes.tick_params(axis='both', which='major', labelsize=15)
        #self.canvas.resize(1024,576)
        self.canvas.draw()

    # MoCap ---------------------------------------------------------------------------------------------------------------------------------------------
    def mocapControl(self):
        print("Control mode: "+str(self.sender().text()))
        whichSketch = "mocapControl"
        self.compileAndUpload(whichSketch)
        if self.manualOn:
            self.manualOff()
        elif self.gestureOn:
            self.gesturesOff()
        elif self.emgOn:
            self.emgOff()
        elif self.mocapOn:
            self.mocapOff()

        self.mocapOn = True
        self.FeedLabel = QLabel()
        self.layout.addWidget(self.FeedLabel, 2, 1, 6, 4, Qt.AlignmentFlag.AlignCenter)
        
        self.Worker = Worker()
        self.Worker.start()
        self.Worker.ImageUpdate.connect(self.ImageUpdateSlot)
        self.setLayout(self.layout)

    def ImageUpdateSlot(self, Image):
        self.FeedLabel.setPixmap(QPixmap.fromImage(Image))

    def CancelFeed(self):
        self.Worker.stop()
        self.FeedLabel.setParent(None)

# Motion capture ----------------------------------------------------------------
class Worker(QThread):
    
    ImageUpdate = pyqtSignal(QImage)

    def __init__(self):
        super(Worker, self).__init__()

        self.cap = cv2.VideoCapture(0)
        self.cap.set(3, 1920) # frameWidth
        self.cap.set(4, 1080) # frameHeight

        self.pTime = 0
        self.cTime = 0

        # Hand tracking
        self.detector = HandDetector(maxHands=1, detectionCon=1)
        # Arduino
        self.arduino = SerialObject("/dev/ttyACM0", 9600,3) # <- change to be able to use other port
        # Range of finger movements
        self.maxDistances = [None] * 5
        self.minDistances = [None] * 5
        self.maxBoxW = 0
        self.maxBoxH = 0
        self.minBoxW = 0
        self.minBoxH = 0
        self.commandExecuted = False
        self.counter = 0

    def run(self):
        self.ThreadActive = True
        Capture = cv2.VideoCapture(0)
        while self.ThreadActive:
            # Get image frame
            success, img = self.cap.read()

            # Find the hand and its landmarks
            img = self.detector.findHands(img)
            lmList, bbox = self.detector.findPosition(img)
            self.cTime = time.time()
            fps = 1 / (self.cTime - self.pTime)
            self.pTime = self.cTime

            # Achieve continuous motion capture of finger movements -------------------------------------------------
            if lmList:
                nfingers = 6
                distance = [None] * nfingers
                # Get distance between landmarks
                distance[0] = int(self.detector.findDistance(4,17,img,False)[0])
                distance[1] = int(self.detector.findDistance(8,0,img,False)[0])
                distance[2] = int(self.detector.findDistance(12,0,img,False)[0])
                distance[3] = int(self.detector.findDistance(16,0,img,False)[0])
                distance[4] = int(self.detector.findDistance(20,0,img,False)[0])
                distance[5],img_unused,lm_rotation = self.detector.findDistance(5,17,img,False)

                if lm_rotation[0] <= lm_rotation[2]:
                    distance[5] = -1 * distance[5]
                
                self.counter += 1

                for i in range(nfingers):
                    # First frame - just assign values
                    if not self.commandExecuted:
                        self.maxDistances = distance
                        self.minDistances = [x-1 for x in distance]
                        print(self.maxDistances)
                        print(self.minDistances)
                        self.commandExecuted = True
                        break
                    
                    # This counteracts (slow) movement relative to the camera plane. Imagine you open your hand right in front of the camera and then move it away, then the distance between the two landmarks used for determining finger flexion is also getting smaller. Therefore minDistance and maxDistance need to adapt for this. Adjust '%100' in the line above to control how often it happens. Additionally, the two next lines determine how adaptive this is.  
                    if self.counter % 50 == 0:
                        self.maxDistances[i] = self.maxDistances[i] * 0.8
                        if i < 5:
                            self.minDistances[i] = self.minDistances[i] * 1.1
                        else:
                            self.minDistances[i] = self.minDistances[i] * 0.8 # minDistance for wrist should go towards zero instead
                    # Keep track of full finger flexion and extension
                    elif distance[i] > self.maxDistances[i]:
                        self.maxDistances[i] = distance[i]
                    elif distance[i] < self.minDistances[i]:
                        self.minDistances[i] = distance[i]
                    
                    # Normalize to range of servos [0,180]
                    normalized_distances = []
                    for j in range(nfingers):
                        # To prevent a bug that occurs sometimes
                        if (self.maxDistances[j] - self.minDistances[j]) <= 0:
                            self.maxDistances[j] *= 1.2
                            self.minDistances[j] *= 0.7
                            break
                        # Normalize thumb to value between 85 and 180
                        if j == 0:
                            normalized_value = 85 + (180-85) * (distance[j] - self.minDistances[j]) / (self.maxDistances[j] - self.minDistances[j])
                        # Normalize all other fingers to a value between 0 and 180
                        else:
                            normalized_value = 180 * (distance[j] - self.minDistances[j]) / (self.maxDistances[j] - self.minDistances[j])
                        
                        # Append the normalized value to the list
                        normalized_distances.append(int(normalized_value))
                        print(normalized_distances)
                    
                if self.counter > 1 and not self.counter % 50 == 0:
                    self.arduino.sendData(normalized_distances) # Send data to Arduino

            # --------------------------------------------------------------------------------------------------
            # Discrete motion capture of finger movements (check how many fingers are up or down)
            if lmList:
                fingers = self.detector.fingersUp()
                #print(fingers)
                #self.arduino.sendData(fingers) # Send data to Arduino

                # To switch back to the discrete finger detection: 1) in l. 179 Use sketch "moCap"
                #                                                  2) Comment out 9 lines above and insted comment in 4 lines above --> change what is sent to Arduino

            # --------------------------------------------------------------------------------------------------
            
            if success:
                Image = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                FlippedImage = cv2.flip(Image, 1)
                FpsImage = cv2.putText(FlippedImage, str(int(fps)), (1220,40), cv2.FONT_HERSHEY_SIMPLEX, 1, (173,216,230), 3)
                ConvertToQtFormat = QImage(FpsImage.data, FpsImage.shape[1], FpsImage.shape[0], QImage.Format.Format_RGB888)
                self.Pic = ConvertToQtFormat.scaled(1280, 720, Qt.AspectRatioMode.KeepAspectRatio)
                
                self.ImageUpdate.emit(self.Pic)

    def stop(self):
        self.ThreadActive = False
        time.sleep(1.5)
        self.cap.release()
        del self.cap

app = QApplication(sys.argv)
window = MainWindow()
window.setStyleSheet(""" font-size: 18px; """)
window.showMaximized()
sys.exit(app.exec())