# Smart Access Control System

*Leveraging IoT and Machine Learning for Secure Authentication*

## Overview

This project presents a next-generation Smart Access Control System that integrates Internet of Things (IoT), machine learning, and secure communication to create a scalable, user-centric alternative to traditional access control methods such as keypads and RFID.

The system consists of three core components:

* **Presence Detection Module**: Detects motion to initiate access flow.
* **Face Recognition Unit**: Performs on-device biometric authentication via ESP32-CAM.
* **Smart Lock & Alert Unit**: Controls physical locking and sends real-time alerts.

A dedicated smartphone application provides role-based access for Residents, Visitors, Technicians, and Security personnel. The system uses the MQTT protocol with AES-256 encryption and TLS for secure communication.

## Features

* Biometric authentication using face recognition (on-device and cloud ML).
* Role-based mobile app with real-time alerts and access logs.
* Smart anomaly detection using behavioral patterns and ML.
* Privacy-first architecture with encrypted communications and data minimization.
* Energy-efficient and scalable design using optimized edge computing.

## Technologies

* ESP32 & ESP32-CAM microcontrollers
* MQTT protocol over TLS
* TensorFlow Lite (Quantized)
* MobileNetV2 (for transfer learning)
* Mobile app with role-based access control
* GDPR-compliant data handling

## Evaluation Metrics

* **FAR ≤ 0.1%** | **FRR ≤ 2%**
* Authentication latency: **≤ 1 second**
* Equal Error Rate (EER): **≈ 1.5%**
