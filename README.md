PyQt6 GUI and Arduino sketches to control a prosthetic hand (inmoov [1]).

Four control modes:
- Servos: Sliders to individually directly control the degree to which each servo is moved
- Gestures: Pre-defined gestures that can be executed with one click
- EMG: State machine based on EMG sensor input to control hand openeing & closing as well as pronation & supination 
- Motion capture: Flexion and extension of our own fingers in front of laptop's webcam is transmitted to control the prosthetic hand's fingers

Upon choosing a control mode in the GUI, the corresponding Arduino sketch is uploaded via the Arduino CLI. 

[1] https://inmoov.fr/inmoov-hand/
