# EDMO Mock Robot Simulator

A complete software simulation of an EDMO robot that implements the full communication protocol used by real hardware. This tool allows you to test and develop the EDMO server system without needing physical robot hardware.

## Features

✅ **Full Protocol Implementation**
- Responds to all EDMO packet types (Identify, SessionStart, SessionEnd, GetTime, UpdateOscillator, etc.)
- Handles packet framing (ED...MO headers/footers) and escaping
- Simulates oscillator behavior with proper phase calculations
- Generates realistic IMU sensor data (gyroscope, accelerometer, magnetometer, gravity, rotation)

✅ **Realistic Simulation**
- Updates oscillator states at 10Hz (same as real hardware)
- Simulates servo motor positions based on frequency, amplitude, offset, and phase
- Generates subtle IMU sensor noise and variations
- Maintains proper timing synchronization with server

✅ **Multi-Robot Support**
- Run multiple simulators simultaneously
- Each with unique identifier and configuration
- Configure oscillator count (4, 6, 8, etc.)

✅ **Network Communication**
- UDP broadcast protocol on port 2121
- Automatic discovery by ServerVNext
- Compatible with the existing EDMOConnectionManager

## Usage

### Building

```powershell
# From the ServerVNext directory
dotnet build EDMOMockSimulator

# Or build the entire solution
dotnet build ServerVNext.sln
```

### Running

**Basic usage (default settings):**
```powershell
cd ServerVNext\EDMOMockSimulator\bin\Debug\net9.0
.\EDMOMockSimulator.exe
```

This will create a robot named "Snake1" with 4 oscillators on UDP port 2121.

**Custom configuration:**
```powershell
# Create a robot named "Snake2" with 6 oscillators
.\EDMOMockSimulator.exe --name Snake2 --oscillators 6

# Short form
.\EDMOMockSimulator.exe -n TestBot -o 8 -p 2121
```

**Run multiple robots:**
```powershell
# Terminal 1
.\EDMOMockSimulator.exe --name Snake1 --oscillators 4

# Terminal 2
.\EDMOMockSimulator.exe --name Snake2 --oscillators 6

# Terminal 3
.\EDMOMockSimulator.exe --name Spider1 --oscillators 8
```

### Command Line Options

| Option | Short | Description | Default |
|--------|-------|-------------|---------|
| `--name` | `-n` | Robot identifier/name | Snake1 |
| `--oscillators` | `-o` | Number of oscillators (motors) | 4 |
| `--port` | `-p` | UDP port to listen on | 2121 |
| `--help` | `-h` | Show help message | - |

## Testing with ServerVNext

### Step 1: Start the Mock Robot(s)

```powershell
cd ServerVNext\EDMOMockSimulator\bin\Debug\net9.0
.\EDMOMockSimulator.exe --name Snake1
```

You should see:
```
╔════════════════════════════════════════════════════════════╗
║         EDMO Mock Robot Simulator                         ║
║         Simulates real EDMO hardware for testing          ║
╚════════════════════════════════════════════════════════════╝

Configuration:
  Robot Name:       Snake1
  Oscillators:      4
  UDP Port:         2121

🤖 Mock EDMO 'Snake1' started on UDP port 2121
👂 Listening for server commands...
✅ Mock robot is running. Press Ctrl+C to stop.
```

### Step 2: Start the EDMO Server

```powershell
# From the repository root
docker compose up
```

Or run directly:
```powershell
cd ServerVNext\EDMOFrontend
dotnet run
```

### Step 3: Connect via Browser

1. Open your browser to `http://localhost:8080`
2. You should see the mock robot(s) in the "Select your EDMO" section
3. Enter your name, select the robot, and click "Let's start!"
4. Control the robot from the interactive controller page

### Expected Behavior

When the server connects, you'll see in the mock simulator console:
```
🔗 Connected to server at 127.0.0.1:xxxxx
📨 Sent identification: Snake1
▶️  Session started
🔄 Updated oscillator 0: F=1.50Hz A=0.75
🔄 Updated oscillator 1: F=1.20Hz A=0.60
...
```

## Architecture

The mock simulator implements the same communication protocol as the Arduino firmware:

```
┌─────────────────────┐
│   ServerVNext       │
│   (EDMOFrontend)    │
└──────────┬──────────┘
           │
           │ UDP Port 2121
           │
           ▼
┌─────────────────────┐
│  MockEDMORobot      │
│  - Responds to      │
│    identification   │
│  - Processes motor  │
│    commands         │
│  - Sends IMU data   │
│  - Simulates        │
│    oscillators      │
└─────────────────────┘
```

## Protocol Details

The mock implements all packet types from `EDMOPacketType`:

- **Identify (0x00)**: Returns robot name, oscillator count, and color hues
- **SessionStart (0x01)**: Begins data streaming, syncs timestamps
- **SessionEnd (0x06)**: Stops data streaming
- **GetTime (0x02)**: Returns current timestamp
- **UpdateOscillator (0x03)**: Updates oscillator parameters (frequency, amplitude, offset, phase shift)
- **SendMotorData (0x04)**: Sends current state of all oscillators
- **SendImuData (0x05)**: Sends IMU sensor readings
- **SendAllData (0x45)**: Sends timestamp + all oscillators + IMU data in one packet

## Differences from Real Hardware

| Feature | Real Robot | Mock Simulator |
|---------|-----------|----------------|
| **Communication** | Serial USB + UDP | UDP only |
| **IMU Data** | Real BNO055 sensor | Simulated with noise |
| **Motor Control** | Physical servos | Mathematical simulation |
| **Power Management** | Battery dependent | Always available |
| **Response Time** | ~10-50ms | <1ms |
| **Data Quality** | Subject to environmental factors | Perfectly consistent |

## Development

The simulator is built using:
- **.NET 9** - Modern C# runtime
- **ServerCore library** - Shared protocol definitions
- **UDP networking** - Same as real robots
- **Binary marshalling** - Exact byte-level compatibility

Key classes:
- `MockEDMORobot.cs` - Main simulation logic
- `Program.cs` - CLI entry point and configuration

## Troubleshooting

**Mock robot not appearing in server:**
- Ensure both are on the same network/machine
- Check that port 2121 is not blocked by firewall
- Verify the server is listening (check EDMOConnectionManager logs)

**Connection drops:**
- Keep the mock simulator running in the foreground
- Check for port conflicts if running multiple instances

**Performance issues:**
- Mock sends data at 10Hz by default
- Reduce oscillator count for lower overhead
- Run on local machine for best performance

## License

Same as ServerVNext - MIT License
