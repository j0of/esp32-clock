# ESP32 Clock
A simple ESP32-powered LCD alarm clock that tells the time and weather via HTTP/APIs
![WhatsApp Image 2025-09-03 at 19 07 51_e4f672a0](https://github.com/user-attachments/assets/a9f9fa02-0c44-4bfd-8781-66c32c62df6d)

# Demo
[Time and weather display](https://github.com/user-attachments/assets/3de79c3b-7c61-450d-a951-33f927ef5e0c)

[Timed alarm](https://github.com/user-attachments/assets/bc7419db-340c-49e4-ab59-87e5d2a50fce)

# Components
- ESP32 (WROOM32 devkit)
- Jumpers
- Passive buzzer
- Potentiometer
- LCD 16x2
- 10k ohm resistor
- Push button
  
# Software
Build and upload using PlatformIO (VSCode extension)

# Wiring
(my attempt at remaking the circuit in Wowki)
<img width="998" height="822" alt="Screenshot 2025-09-03 185047" src="https://github.com/user-attachments/assets/9b67950b-69e4-4a77-be34-96ddd797af86" />

- LCD GND -> GND
- LCD VDD -> 5V
- LCD VO -> Potentiometer (middle)
- LCD RS -> ESP D22
- LCD RW -> GND
- LCD E -> ESP D23
- LCD D4 -> ESP D14
- LCD D5 -> ESP D15
- LCD D6 -> ESP D16
- LCD D7 -> ESP D17
- Potentiometer (left) -> 5V
- Potentiometer (right) -> GND
- Push button (first leg) -> 5V
- Push button (second leg) -> ESP D33 -> (10k ohm resistor) GND
- Passive buzzer (+) -> ESP D21
- Passive buzzer (-) -> GND
