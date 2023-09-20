# **Prosthetic hand**
The goal of this project was to build a prosthetic hand. Over time I extended it way beyond that initial intent. At this stage, the project entails the prosthetic hand, a Linux GUI, an Android app (not yet in this repo), and 4 distinct possibilities for controlling the prosthetic via the GUI. These control modes are:

#### Servos
Direct control of each of the 6 servos individually. Possible values for the index, middle, ring, and pinky fingers are in the interval [0,180], where 0 is full flexion and 180 is full extension. To avoid possible collisions of the thumb with the index finger, the thumbs’ maximal flexion is limited to 60, therefore the thumb servos' control interval is [60,180]. For the wrist, pronation is 180 and supination is 0.
![servos](/img/servos.jpg)

#### Gestures
It's possible to choose one of currently 17 predefined gestures. When selecting a gesture it will be executed. After 5 s the prosthetic hand will move back to the neutral position to avoid being under tension unnecessarily long.
![gestures](/img/gestures.jpg)

#### EMG
I implemented this part to give an intuition of how prosthetics that are actually in use nowadays work. These are of course significantly more sophisticated. The principle that users are often required to learn arbitrary mappings between muscle contractions and prosthetic hand movements holds, however. And making all of this much more intuitive is an incredibly important aspect of research in this field (imho). Again, I am aware that multi-electrode systems with machine learning, targeted muscle reinnervation (TMR), etc. exist. Now let's go on to how I implemented the EMG control as a state machine here. Contracting the muscle once closes the hand, contracting it twice in quick succession (i.e. 2 supra-threshold contractions within 800 ms) opens the hand. It is furthermore possible to switch the state from controlling hand opening and closing to controlling wrist pronation and supination. Switching between these states is achieved by holding a supra-threshold contraction for 2 s. In this state, one contraction results in wrist pronation, while 2 quick contractions (again within 500 ms) result in supination. Holding another supra-threshold contraction for at least 2 s then switches back to controlling hand opening and closing. The threshold value that has to be surpassed can be controlled with a slider in the GUI to quickly adapt to a person's individual EMG signal, which is dependent on factors such as environmental noise, skin moisture, contraction force, etc.
![emg](/img/emg.jpg)

#### Motion capture
The fourth control mode is moCap. The prosthetic hand replicates finger flexion and extension that are tracked through the laptop's webcam. Here, I integrated an OpenCV implementation for motion capture. If discrete detection is used, a stream of five integers is continuously sent to the Arduino, where a '1' indicates that the finger is extended while a '0' is sent when a finger is flexed. Recently I implemented the continuous (=proportional) tracking of finger and wrist movements. (To go back to discrete a few things have to be changed.)
![moCap](/img/moCap.jpg)

#### Arduino CLI
Whenever the user switches between the four control modes in the GUI by clicking one of the buttons in the top panel, the corresponding sketch is uploaded to the Arduino through the command line interface (CLI).
## Usage
Navigate in a shell to the current project.
```console
conda env create -n ENVNAME --file prostheticHand.yml
```
Attach Arduino via USB cable to Laptop/PC.

Run 'prostheticHand.py' to use the GUI (I'm running it in VSCode).

*Troubleshoot*:
- l.91 change if you're not using an Arduino Mega
- l. 106 if the Arduino port is not found automatically
- l. 675 try another integer (1,2,...) if 0 isn't your webcam
## Folder structure
```
│projectdir
|
├── emgControl           <- Reading EMG signal, thresholding and state machine (Arduino sketch)
|
├── manualControl        <- Servos (sliders) and gestures (Arduino sketch)
|
├── motionCapture        <- Motion capture via OpenCV (Arduino sketch)
|
├── img                  <- Images for software, software icon, and this README.md
|
├── parts_list           <- List of parts I ordered for this project
|
├── prostheticHand.py    <- GUI using PyQt6
|
├── prostheticHand.yml   <- conda environment
│
├── README.md
```

## Sources
(1) The prosthetic hand is the [inmoov hand i2](https://inmoov.fr/hand-i2/) and part of the fantastic open-source [inmoov](https://inmoov.fr/) project by Gael Langevin. 'appIcon.png' and 'inmoov_no_bg.pg' are also from the inmoov website. I made changes to the following parts:
- i2_FingersMoldX5V3.stl: Remove part of the lid because the silicone didn't dry
- i2_FingersTipX5V2.stl: Moved the hole for the screw where the springs are attached because it kept breaking at the very small plastic part below
- i2_WristGearV1.stl: Made the spikes of the part that gets attached to the servo fit the servo gear better, because they wore off too easily.

(2) https://www.youtube.com/watch?v=7KV5489rL3c
