# PiDroneUI  

C++ wxWidgets drone simulator with real-time telemetry visualization and Raspberry Pi socket-based data streaming.  

---

## ✨ Features  
- Real-time telemetry dashboard (position, velocity, orientation)  
- 2D map interface with interactive visualization  
- TCP socket communication with Raspberry Pi hardware  
- Modular design for simulated or live sensor data input  

---

## 🛠️ Tech Stack  
- **Language:** C++  
- **UI Framework:** wxWidgets  
- **Networking:** BSD Sockets (TCP)  
- **Hardware:** Raspberry Pi (data source)  

---

## 🚀 Getting Started  

### Prerequisites  
- C++17 or later  
- wxWidgets installed  
- Raspberry Pi (optional, for live sensor streaming)  

### Build Instructions  
```bash
# Clone repository
git clone https://github.com/yourusername/PiDroneUI.git  
cd PiDroneUI  

# Build with g++
g++ main.cpp -o PiDroneUI `wx-config --cxxflags --libs`  
