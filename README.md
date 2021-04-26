# mubone

A mubone is a kind of trombone where instead of being manipulated directly with
the hand, the slide is moved via a digital controller. As well as several
buttons and potentiometers on the controller, a mubone also senses the
orientation of the controller relative to the brass, the orientation of the
brass relative to the world, and the position of the slide. Mubones are like
trombones; they are a type of instrument that anyone can, in principle, make and
play. There is not a single capital-M "Mubone".

This repository presents the implementation of the most recent incarnation of
this overall instrument identity, designed by Travis West and Kalun Leung.
This mubone uses the following sensors to achieve the broad design identity of
a mubone as described above:

- slide position sensor: slide position is sensed with a string potentiometer
- the orientation of the brass with respect to (wrt) the world is measured by
  a magnetic-inertial measurement unit (MIMU)
- the orientation of the controller wrt the brass is measured by a series of
  three potentiometers aligned with the coordinate axes of the controller, which
  are embedded in the mechanical linkage between the controller and the slide
- the controller presents the following additional control elements:
    - four buttons and a joystick on the top surface
    - eight RGB LEDs on the top surface
    - a trigger-actuated potentiometer played with the index finger
    - three buttons along the grip, played with the middle, ring, and small fingers

This document presents a broad overview of the implementation. More detail on
specific aspects can be found in the appropriate sub-folders (firmware,
granular, etc).

## sensors

### slide position sensor

The slide position sensor is a kind of string potentiometer. A Kevlar tether is
attached to a fixed point on the brass near the mouthpiece, and wrapped around
a spring-loaded spool. When the slide is extended, the spool rotates freely
releasing the tether. When the slide is retracted, the spring causes the spool
to turn back and take up the tether. A shaft is attached to the spool and
turns two potentiometers. By measuring the rotation of the spool, the position
of the slide can be tracked. Two potentiometers are used since a single
potentiometer has a dead zone of about 30 degrees where the measurement is not
accurate.

### orientation sensor

An MIMU integrates three accelerometers, three gyroscopes, and three magnetic
field sensors to allow simultaneous measurement of magnetic north, angular rate,
and gravity vectors; the sensor unit is also called a MARG sensor based on
these three measured quantities. When the three measurements are combined with
an appropriate sensor fusion algorithm, the orientation of the sensor wrt the
global coordinate system can be derived. In order to provide accurate and
stable measurements, the sensors also need to be calibrated. The magnetic
sensor is especially prone to large errors due to the presence of ferromagnetic
materials nearby the sensor (e.g. nuts and bolts in the assembly) and in the
local environment (e.g. structural beams).

### joint sensors

The mechanical joint connecting the controller to the slide consists of
rotating axis concealed under the bell-shaped façade attached to the slide, and
another pair of rotating axes integrated into a universal-joint (u-joint) in
between the bell façade and the hand-held controller. Potentiometers are
integrated at each of these axes: the rotator under the bell façade, and the
two axes of the u-joint.

### controller

On top of the controller there are four push-buttons, eight LEDs, and a joystick
with integrated push-button. Along the grip, there are three more push-buttons
and a trigger. The trigger turns a potentiometer, and is designed to allow a
wide range of motion. The trigger is loaded with a simple 3D printed spring to
return it to a neutral position when not pressed. The design permits the
trigger to be pulled back slightly with the finger as well as being pushed in
the expected manner.

## electronic design

### circuit boards

The design incorporates eight different printed circuit boards (PCBs) connected
by flexible ribbon cables. This approach was chosen to facilitate ongoing
improvement to the design without having to replace the whole electronic
subassembly with every update. In addition to the principle PCB (the
motherboard) that includes the microcontroller unit (MCU) that reads the
sensors, the following elements have their own circuit boards as well:

- the slide position sensor
- the rotator (including the MIMU)
- the u-joint
- the top buttons and LEDs
- the top joystick
- the trigger
- the grip buttons

### high resolution analog-digital converters

ADS1219 analog-to-digital converters (ADCs) are used to measure the signals
from the trigger, u-joint, rotator, and slide position sensor. This integrated
circuit (IC) allows two 24-bit bi-polar signals or four 23-bit uni-polar
signals to be captured with a reasonable sampling rate. The IC is controlled
using I2C protocol. One is placed on the PCB for the slide position sensor and
is solely used to measure the signals from its two potentiometers. Another is
placed on the motherboard and measures the signals from the rotator, u-joint,
and trigger potentiometers. The remaining analog signals from the joystick are
measured using the 10-bit converts integrated in the MCU.

The choice to use high resolution ADCs was motivated by our experience with an
earlier incarnation of the mubone design, where we found that we could perceive
the steps between bits when slowly turning the controller--the resolution of
our ability to control the mubone was greater than the resolution of the
sensors in it.  We considered this situation unacceptable, thus the selection
of high resolution ADCs. We had no such issues with the joystick, so we
retained the MCU's converters for its signals.

### battery power

The controller assembly is powered by a single rechargeable 2200 mAh 3.7 V
cylindrical lithium battery. The battery is attached to the grip, where its
mass helps to counterbalance that of the rest of the attachment. Currently it
remains future work to integrate such niceties as a charger and fuel gauge into
the design.

### wireless communications

An ESP8266 module is connected to the MCU via the motherboard. Its firmware is
configured to receive serial line internet protocol (SLIP) encoded open sound
control (OSC) packets (i.e. SLIP-encoded OSC packets) from the MCU and forward
them to the local area network. A seperate module is used to offload the network
handling from the MCU so that the latter can focus on reading sensors and
conditioning their signals, as well as to somewhat simplify prototyping.

## applications

### sensor model

This JUCE application reads the sensors over the network and conditions,
combines, and analyses the raw data to produce additional useful gesture
features. The raw data and analysed features are then re-broadcast over OSC for
further use by other applications.

As much as possible, this application is used as a rapid prototyping tool for
gesture-feature extraction algorithms that can ultimately be integrated in the
device firmware. 

### granular

This JUCE application creates a spatial recorder and granulator allowing sounds
to be placed at locations in space around the player and later granulated by
pointing the mubone towards those locations. This is the original mubone
application. A pure data patch is used to achieve the mapping, so in principle
the granulator could be used with any controller that provides a spatial signal
to use for positioning and recalling sounds.

### pure data patches

As well as the mapping for the granular application, several other pure data
patches are included in the repository. These are mostly sketches from
workshops exploring other possible approaches to using the mubone. 
