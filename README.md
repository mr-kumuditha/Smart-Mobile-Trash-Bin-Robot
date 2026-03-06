# Smart Mobile Trash Bin Robot

A smart mobile trash bin robot designed to improve waste disposal efficiency in indoor environments such as canteens, corridors, and public spaces. The robot moves autonomously and allows users to dispose waste easily without searching for a fixed bin.

The system combines robotics, sensors, and automation to create a mobile waste collection solution that improves cleanliness and convenience.

---

## Project Overview

In large canteen areas and public indoor environments, trash bins are usually placed in fixed locations. Many people do not walk to these bins, which results in waste being left on tables or floors.

This project introduces a mobile trash bin robot that moves around the environment and provides easy access for waste disposal. The robot stops when a user is detected, opens the lid automatically, and continues moving after the waste is deposited.

---

## Key Features

• Dual navigation modes  
• Line following navigation using floor path  
• Random obstacle avoidance mode  
• Automatic lid opening using servo motor  
• User detection using ultrasonic sensor  
• Trash level monitoring inside the bin  
• LCD display showing system status  
• LED indicators for system feedback  
• Battery powered mobile platform  
• Mode selection using physical switch  

---

## Navigation Modes

### Line Following Mode
The robot follows a predefined black line path on the floor. This mode is useful for controlled environments where the robot follows a specific route.

### Random Obstacle Avoidance Mode
The robot moves freely and uses an ultrasonic sensor to detect and avoid obstacles while navigating the environment.

---

## Hardware Components

Main components used in this project:

Arduino UNO  
Arduino Nano  
Ultrasonic Sensors  
IR Line Sensors  
Servo Motor (Lid Control)  
LCD Display (I2C)  
TB6612FNG Motor Driver  
DC Geared Motors  
LED Indicators  
Buzzer  
18650 Battery Pack  
5V Buck Converter  
Chassis with wheels  

---

## System Architecture

The Arduino UNO acts as the main controller and processes data from sensors to control movement and lid operation.

Sensors detect the environment and user interaction while the motor driver controls the robot movement. A servo motor operates the trash bin lid, and an LCD display shows system information such as navigation mode and bin status.

---

## Circuit Diagram

The circuit was designed using a simulation wiring tool and includes connections for sensors, motor drivers, display, and power management.

Add your circuit image here:

![Circuit Diagram](circuit_diagram.png)

---

## Working Logic

1. Robot powers on
2. System checks selected navigation mode
3. Robot starts moving based on the selected mode
4. Ultrasonic sensor checks for user presence
5. If a user is detected
6. Robot stops
7. Lid opens automatically
8. User deposits waste
9. Lid closes after a few seconds
10. Robot continues navigation

---

## Applications

Indoor waste management systems  
University canteens  
Smart campuses  
Office environments  
Research and robotics education  

---

## Project Structure
