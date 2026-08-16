# 🚗 IoT-Based Smart Vehicle Access System

<p align="center">
  <img src="picture 1.png" alt="Smart Vehicle Access System" width="850">
</p>

<p align="center">
  <b>Arduino Mega + ESP32 + Firebase Realtime Database</b>
</p>

<p align="center">
  An IoT-based vehicle authentication, gate control and cloud logging prototype.
</p>

---

## 📌 Project Overview

The **IoT-Based Smart Vehicle Access System** is an automated vehicle access and gate-control prototype.

The system uses an **Arduino Mega** as the main hardware controller and an **ESP32** as the IoT gateway. The Arduino Mega handles vehicle detection, keypad input, PIN authentication, LCD messages, LEDs, buzzer, servo gates, entry/exit logic and local user management. The ESP32 receives vehicle information from the Mega through serial communication, obtains date and time using NTP, and sends the information to **Firebase Realtime Database** over Wi-Fi.

The project is designed to demonstrate secure vehicle access control together with cloud-based monitoring and logging.

---

## ✨ Main Features

- 🚗 Vehicle detection using an ultrasonic sensor
- 🔐 5-digit PIN-based authentication for registered users
- 👤 User name, role, vehicle plate and vehicle type stored in the controller
- 🚪 Separate Entry and Exit servo gates
- 🔄 Automatic Entry/Exit recognition using the user's current inside/outside status
- 🔴 Wrong PIN indication using red LED and buzzer
- 🔒 System lock for 15 seconds after 3 consecutive wrong PIN attempts
- ⌨️ Keypad-based PIN input with `#` for confirmation and `*` for clearing
- ⏱️ PIN input timeout after 15 seconds
- 🛡️ Duplicate PIN-entry protection with a 5-second cooldown
- 👨‍💼 Admin mode using a special PIN
- ➕ Admin can add a guest user
- 🗑️ Admin can delete an existing user
- 📡 Arduino Mega ↔ ESP32 serial communication
- 🌐 ESP32 Wi-Fi connectivity
- ☁️ Firebase Realtime Database integration
- 🕒 NTP-based date and time
- 📊 Parking overview data sent to Firebase
- 📝 Vehicle event logs pushed to Firebase
- 🪪 RFID card-based vehicle/user identification (planned extension)

---

## 🏗️ System Architecture

```text
                    🚗 VEHICLE
                         │
                         ▼
                Ultrasonic Sensor
                         │
                         ▼
                  Arduino Mega
                         │
       ┌─────────────────┼──────────────────┐
       │                 │                  │
       ▼                 ▼                  ▼                 ▼                  ▼
    Keypad            RFID Reader        Servo Gates
       │                 │
       └──────────┬──────┘
                  ▼
            Authentication
       │                 │                  │
       ▼                 │                  │
 PIN Authentication      │                  │
       │                 │                  │
       └─────────────────┼──────────────────┘
                         │
                         ▼
                  Entry / Exit Logic
                         │
                         │ JSON
                         ▼
                       ESP32
                         │
                ┌────────┼────────┐
                │        │        │
              Wi-Fi     NTP    JSON Processing
                │        │        │
                └────────┼────────┘
                         ▼
                  Firebase RTDB
                    │          │
                    ▼          ▼
                /overview  /vehicle_logs
```

---

## 🔄 System Workflow

### 1. Vehicle Detection

When a vehicle comes near the gate, the ultrasonic sensor measures the distance.

The current Mega code considers a vehicle present when the measured distance is:

```text
greater than 0 cm and less than 6 cm
```

When no vehicle is detected, the LCD shows a waiting message and the blue LED is activated.

When a vehicle arrives, the LCD asks the user to enter a PIN and the green LED is activated.

---

### 2. PIN Input

The user enters the PIN using the 4×3 keypad.

- Number keys → enter PIN digits
- `#` → submit PIN
- `*` → clear entered PIN
- Maximum normal PIN input → 5 digits

The entered PIN is displayed on the LCD as asterisks.

---

### 3. PIN Verification

The Arduino Mega searches the active local user list for a matching PIN.

Each user record contains:

```text
PIN
Name
Role
Vehicle Plate
Vehicle Type
Inside/Outside Status
Active/Inactive Status
```

The current code contains predefined users and supports up to:

```text
MAX_USERS = 15
```

---

### 4. Valid PIN

When a valid PIN is entered:

- Wrong-attempt counter is reset
- The user's current inside/outside state is checked
- Entry or Exit is determined
- The corresponding gate is operated
- Vehicle information is sent to the ESP32

---

### 5. Entry Logic

If the authenticated user is currently outside:

```text
Outside
   ↓
Entry
   ↓
Entry counter +1
   ↓
User status → Inside
   ↓
Entry Gate operates
   ↓
Vehicle data → ESP32
```

The LCD displays the user's role and a welcome message.

---

### 6. Exit Logic

If the authenticated user is currently inside:

```text
Inside
   ↓
Exit
   ↓
Exit counter +1
   ↓
User status → Outside
   ↓
Exit Gate operates
   ↓
Vehicle data → ESP32
```

The LCD displays the user's role and a goodbye message.

---

## 🔐 Security Features

### Wrong PIN Protection

For an invalid PIN:

- Red LED turns on
- Buzzer produces an alarm
- Wrong-attempt counter increases
- Gate does not operate

### 3-Attempt Lock

After 3 wrong PIN attempts, the system enters a 15-second lock period.

The LCD displays a countdown:

```text
SYSTEM LOCKED!
Wait: 15s
```

The wrong-attempt counter is then reset.

### Input Timeout

If the user starts entering a PIN but does not complete it within 15 seconds, the entered PIN is cleared.

### Duplicate Protection

The same successfully processed PIN is protected against immediate duplicate processing for 5 seconds.

---

## 👨‍💼 Admin Mode

The system has a special Admin PIN:

```text
9999
```

After entering the Admin PIN, the LCD provides:

```text
ADMIN MODE
1:Add  2:Delete
```

### Add Guest

The Admin can add a guest using a 4-digit guest PIN.

The system automatically creates:

```text
Guest-1
Guest-2
Guest-3
...
```

with:

```text
Role = Visitor
Vehicle Type = Car
```

and generates a guest plate identifier.

### Delete User

The Admin can enter an existing 4- or 5-digit PIN and deactivate that user.

> **Important:** The current Admin changes are stored in the Arduino Mega's runtime memory. This code does not implement persistent storage such as EEPROM or a Firebase user-management database.

---

# 🔧 Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| Arduino Mega 2560 | 1 | Main hardware and security controller |
| ESP32 | 1 | Wi-Fi, NTP and Firebase communication |
| Ultrasonic Sensor | 1 in current Mega code | Vehicle detection |
| Servo Motor | 2 | Entry and Exit gate control |
| 4×3 Keypad | 1 | PIN input |
| 16×2 I2C LCD | 1 | System/user messages |
| Green LED | 1 | Vehicle/access status |
| Red LED | 1 | Wrong PIN indication |
| Blue LED | 1 | Waiting/system status |
| Buzzer | 1 | Keypress and security alarm |
| RFID Reader + Card/Tag | 1+ | RFID-based identification (planned extension) |
| Breadboard & Jumper Wires | As required | Circuit connection |

---

# 📌 Arduino Mega Pin Configuration

The following mapping is based on the current Arduino Mega code.

| Device | Mega Pin |
|---|---:|
| Buzzer | D8 |
| Green LED | D9 |
| Red LED | D10 |
| Blue LED | D11 |
| Entry Servo | D5 |
| Exit Servo | D12 |
| Ultrasonic TRIG | D47 |
| Ultrasonic ECHO | D46 |
| Keypad Row 1 | D30 |
| Keypad Row 2 | D31 |
| Keypad Row 3 | D32 |
| Keypad Row 4 | D33 |
| Keypad Column 1 | D34 |
| Keypad Column 2 | D35 |
| Keypad Column 3 | D36 |
| LCD | I2C, address `0x27` |
| Mega → ESP32 serial | `Serial1` |

The Mega code initializes `Serial1` at 9600 baud for communication with the ESP32.

---

# 📡 Arduino Mega ↔ ESP32 Communication

The Arduino Mega sends vehicle information to the ESP32 as a JSON message.

Conceptually, the message contains:

```json
{
  "total": 10,
  "inside": 1,
  "entry": 5,
  "exit": 4,
  "latest_vehicle": {
    "name": "User Name",
    "plate": "ABC-1234",
    "type": "Car",
    "status": "Inside",
    "access": "Entry",
    "category": "Student"
  }
}
```

The ESP32 parses this JSON and uploads the relevant information to Firebase.

### ESP32 Serial Pins

| ESP32 | Function |
|---|---|
| GPIO16 | RX2 |
| GPIO17 | TX2 |
| Baud Rate | 9600 |

> **Hardware note:** Arduino Mega logic is 5 V while ESP32 GPIO is 3.3 V logic. Use an appropriate level-shifting method/voltage divider for the Mega TX → ESP32 RX connection.

---

# ☁️ Firebase Integration

The ESP32 communicates with **Firebase Realtime Database** through Wi-Fi.

Two main database areas are written by the current ESP32 code:

```text
Firebase
│
├── overview
│   ├── total_capacity
│   ├── inside
│   ├── total_entry
│   └── total_exit
│
└── vehicle_logs
    └── generated log IDs
        ├── name
        ├── vehicle_no
        ├── type
        ├── category
        ├── access
        ├── status
        └── timestamp
```

### `/overview`

Stores the current summary:

- Total parking capacity
- Vehicles currently inside
- Total entries
- Total exits

### `/vehicle_logs`

Stores individual vehicle events.

Each event contains:

- Name
- Vehicle number
- Vehicle type
- User category/role
- Entry or Exit action
- Inside/Outside status
- Date and time

---

# 🕒 Date & Time

The ESP32 uses an NTP server:

```text
pool.ntp.org
```

and applies a UTC+6 offset.

The generated timestamp follows this format:

```text
YYYY-MM-DD HH:MM:SS AM/PM
```

Example:

```text
2026-08-16 08:35:20 PM
```

No dedicated RTC module is required for this time synchronization method.

---

# 👥 Current User Data

The current Mega code contains example registered users with:

- 5-digit PINs
- Names
- Roles
- Vehicle plates
- Vehicle types
- Inside/outside state
- Active/inactive state

Example roles include:

```text
Teacher
Guest Teacher
External Examiner
Student
Staff
```

> **Security:** Do not publish real users' names, vehicle numbers, PINs or credentials in a public repository. Replace them with sample/demo data before publishing.

---


# 🪪 RFID Integration

RFID can be added as an additional authentication method alongside the existing keypad PIN system.

A typical RFID workflow is:

```text
RFID Card / Tag
       │
       ▼
 RFID Reader
       │
       ▼
Arduino Mega
       │
       ├── Read UID
       ├── Find registered user
       ├── Check Inside/Outside status
       └── Entry / Exit decision
                │
                ▼
            Servo Gate
                │
                ▼
              ESP32
                │
                ▼
             Firebase
```

### Recommended RFID Features

- Read the RFID card/tag UID
- Match the UID with a registered user
- Show the user's name/role on the LCD
- Allow Entry or Exit after successful authentication
- Trigger the same wrong-access/security indication for unauthorized cards
- Send RFID-based Entry/Exit events to Firebase
- Keep the existing keypad PIN as an alternative authentication method

### Important

**RFID hardware and RFID code are not included in the current uploaded Arduino Mega/ESP32 implementation.** The current source code uses the keypad PIN as the authentication input. Therefore, RFID is documented here as a planned extension until the RFID reader model, wiring, and code are added.

# 💻 Software & Technologies

- **Arduino IDE**
- **C/C++**
- **Arduino Mega 2560**
- **ESP32**
- **Wi-Fi**
- **Firebase Realtime Database**
- **ArduinoJson**
- **FirebaseESP32**
- **NTPClient**
- **WiFiUDP**
- **Keypad Library**
- **Servo Library**
- **LiquidCrystal_I2C Library**

---

# 📁 Repository Structure

```text
IoT-Based-Smart-Vehicle-Access-System/
│
├── README.md
│
├── Arduino_Mega/
│   └── Smart_Vehicle_Access_Mega.ino
│
├── ESP32/
│   └── Firebase_ESP32.ino
│
├── Firebase/
│   ├── database-structure.json
│   └── firebase-rules.json
│
├── Circuit_Diagram/
│   └── circuit-diagram.png
│
├── Project_Images/
│   ├── project-overview.jpg
│   ├── hardware.jpg
│   └── working.jpg
│
└── Documentation/
    └── project-report.pdf
```

---

# ⚙️ Installation & Setup

## 1. Arduino Mega

Install the required Arduino libraries:

```text
Keypad
Servo
Wire
LiquidCrystal_I2C
```

Upload the Mega program to the Arduino Mega 2560.

---

## 2. ESP32

Install the required libraries:

```text
WiFi
FirebaseESP32
ArduinoJson
WiFiUdp
NTPClient
time
```

Before uploading the ESP32 program, configure your own:

```text
Wi-Fi SSID
Wi-Fi Password
Firebase configuration
```

Do not publish real credentials in the GitHub repository.

---

## 3. Firebase

Create a Firebase Realtime Database and configure the required database rules.

Use the database structure documented in:

```text
Firebase/database-structure.json
```

---

## 4. Hardware Connection

Connect the components according to:

```text
Circuit_Diagram/circuit-diagram.png
```

Ensure that the Mega-to-ESP32 serial connection uses safe voltage levels.

---

# 📷 Project Gallery

## Complete Prototype

![Project Overview](Project_Images/project-overview.jpg)

Add additional images here as the project is documented:

```text
Project_Images/
├── project-overview.jpg
├── hardware.jpg
├── circuit.jpg
└── firebase-dashboard.jpg
```

---

# ⚠️ Current Limitations

The current implementation has several prototype-level limitations:

1. The registered user list is stored in the Arduino Mega program.
2. Admin-added users are not persistently stored in Firebase or EEPROM by the current Mega code.
3. The current Mega code uses one ultrasonic sensor for vehicle detection.
4. User PIN authentication is performed against the Mega's local user array rather than directly against Firebase.
5. Firebase logging depends on the ESP32 having Wi-Fi connectivity.
6. The ESP32 code currently pushes vehicle logs and overview data but does not implement a full remote user-management system.
7. Real credentials must be kept outside the public GitHub repository.
8. The project is a prototype and should be further hardened before deployment in a real security-critical environment.

---

# 🔮 Future Improvements

Possible future improvements include:

- 📱 Mobile application
- 🌐 Full web dashboard
- 👨‍💼 Remote user management through Firebase
- 🔐 Stronger authentication and credential management
- 💾 Persistent local storage
- 📷 License Plate Recognition (LPR)
- 🪪 RFID-based vehicle identification
- 🖐️ Fingerprint authentication
- 🚗 Multiple parking-slot detection
- 📊 Advanced parking analytics
- 🔔 Remote notifications
- 🔋 Better power management
- 🛡️ Improved fail-safe gate control

---

# 🎯 Project Objective

The main objective of this project is to combine:

```text
Embedded System
       +
IoT Connectivity
       +
Cloud Database
       +
Vehicle Authentication
       +
Automatic Gate Control
```

into one integrated smart vehicle access system.

---

# 👨‍💻 Developer

**Ashikur Rahman**

IoT-Based Smart Vehicle Access System

---

# 📄 License

This project is intended for educational, academic and prototype development purposes.

If you reuse or modify this project, please provide appropriate credit to the original project.
