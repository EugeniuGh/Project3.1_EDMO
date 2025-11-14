using System.Net;
using System.Net.Sockets;
using System.Numerics;
using System.Runtime.InteropServices;
using ServerCore.EDMO.Communication;
using ServerCore.EDMO.Communication.Commands;
using ServerCore.EDMO.Communication.Packets;

namespace EDMOMockSimulator;

/// <summary>
/// A complete mock EDMO robot that simulates all communication protocols
/// </summary>
public class MockEDMORobot
{
    private readonly string identifier;
    private readonly int oscillatorCount;
    private readonly int udpPort;
    
    private UdpClient? udpClient;
    private CancellationTokenSource? cancellationTokenSource;
    private Task? listenTask;
    private Task? dataStreamTask;
    
    private OscillatorState[] oscillatorStates;
    private IMUData currentIMUData;
    
    private bool sessionStarted;
    private DateTime sessionStartTime;
    private uint sessionStartOffset;
    
    private IPEndPoint? serverEndpoint;
    
    private readonly Random random = new();

    public CancellationToken CancellationToken => cancellationTokenSource?.Token ?? CancellationToken.None;

    public MockEDMORobot(string identifier, int oscillatorCount, int udpPort)
    {
        this.identifier = identifier;
        this.oscillatorCount = oscillatorCount;
        this.udpPort = udpPort;
        
        InitializeOscillatorStates();
        InitializeIMUData();
    }

    private void InitializeOscillatorStates()
    {
        oscillatorStates = new OscillatorState[oscillatorCount];
        
        for (int i = 0; i < oscillatorCount; i++)
        {
            oscillatorStates[i] = new OscillatorState(
                Frequency: 1.0f,
                Amplitude: 0.5f,
                Offset: 0.5f,
                PhaseShift: i * (float)(Math.PI * 2 / oscillatorCount),
                Phase: 0.0f
            );
        }
    }

    private void InitializeIMUData()
    {
        currentIMUData = new IMUData(
            Gyroscope: new SensorInfo<Vector3>
            {
                Timestamp = 0,
                Accuracy = 3,
                Data = Vector3.Zero
            },
            Accelerometer: new SensorInfo<Vector3>
            {
                Timestamp = 0,
                Accuracy = 3,
                Data = new Vector3(0, 0, 9.81f)
            },
            MagneticField: new SensorInfo<Vector3>
            {
                Timestamp = 0,
                Accuracy = 2,
                Data = new Vector3(0.2f, 0.1f, 0.4f)
            },
            Gravity: new SensorInfo<Vector3>
            {
                Timestamp = 0,
                Accuracy = 3,
                Data = new Vector3(0, 0, 9.81f)
            },
            Rotation: new SensorInfo<Quaternion>
            {
                Timestamp = 0,
                Accuracy = 3,
                Data = Quaternion.Identity
            }
        );
    }

    public void Start()
    {
        if (cancellationTokenSource != null)
        {
            Console.WriteLine("⚠️  Robot is already running!");
            return;
        }

        cancellationTokenSource = new CancellationTokenSource();
        udpClient = new UdpClient(udpPort);
        
        listenTask = Task.Run(ListenForCommands);
        dataStreamTask = Task.Run(SimulateRobotBehavior);
        
        Console.WriteLine($"🤖 Mock EDMO '{identifier}' started on UDP port {udpPort}");
    }

    public void Stop()
    {
        if (cancellationTokenSource == null)
            return;

        cancellationTokenSource.Cancel();
        udpClient?.Close();
        
        try
        {
            Task.WaitAll([listenTask!, dataStreamTask!], TimeSpan.FromSeconds(2));
        }
        catch (AggregateException)
        {
            // Expected on cancellation
        }
        
        Console.WriteLine($"👋 Mock EDMO '{identifier}' stopped");
    }

    private async Task ListenForCommands()
    {
        var ct = cancellationTokenSource!.Token;
        
        Console.WriteLine($"👂 Listening for server commands...");
        
        var buffer = new List<byte>();
        bool isReceivingPacket = false;

        try
        {
            while (!ct.IsCancellationRequested)
            {
                UdpReceiveResult result;
                try
                {
                    result = await udpClient!.ReceiveAsync(ct);
                }
                catch (OperationCanceledException)
                {
                    break;
                }

                // Remember the server endpoint for sending responses
                if (serverEndpoint == null || !serverEndpoint.Equals(result.RemoteEndPoint))
                {
                    serverEndpoint = result.RemoteEndPoint;
                    Console.WriteLine($"🔗 Connected to server at {serverEndpoint}");
                }

                // Process received data byte by byte to handle packet framing
                foreach (byte b in result.Buffer)
                {
                    buffer.Add(b);

                    // Check for packet header "ED"
                    if (buffer.Count >= 2 && buffer[^2] == 'E' && buffer[^1] == 'D')
                    {
                        isReceivingPacket = true;
                        buffer.Clear();
                        buffer.Add((byte)'E');
                        buffer.Add((byte)'D');
                        continue;
                    }

                    // Check for packet footer "MO"
                    if (isReceivingPacket && buffer.Count >= 4 && buffer[^2] == 'M' && buffer[^1] == 'O')
                    {
                        // Extract packet (without ED and MO)
                        var packet = buffer.ToArray()[2..^2];
                        await HandleCommand(packet);
                        
                        buffer.Clear();
                        isReceivingPacket = false;
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"❌ Error in listen loop: {ex.Message}");
        }
    }

    private async Task HandleCommand(byte[] escapedPacket)
    {
        try
        {
            var packet = EDMOPacket.UnescapePacket(escapedPacket);
            
            if (packet.Length == 0)
                return;

            var commandType = (EDMOPacketType)packet[0];
            
            switch (commandType)
            {
                case EDMOPacketType.Identify:
                    await SendIdentification();
                    Console.WriteLine($"📨 Sent identification: {identifier}");
                    break;

                case EDMOPacketType.SessionStart:
                    HandleSessionStart(packet);
                    Console.WriteLine($"▶️  Session started");
                    break;

                case EDMOPacketType.SessionEnd:
                    HandleSessionEnd();
                    Console.WriteLine($"⏹️  Session ended");
                    break;

                case EDMOPacketType.GetTime:
                    await SendCurrentTime();
                    break;

                case EDMOPacketType.UpdateOscillator:
                    HandleOscillatorUpdate(packet);
                    break;

                case EDMOPacketType.SendMotorData:
                    await SendMotorData();
                    break;

                case EDMOPacketType.SendImuData:
                    await SendIMUData();
                    break;

                default:
                    Console.WriteLine($"⚠️  Unknown command: {commandType}");
                    break;
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"❌ Error handling command: {ex.Message}");
        }
    }

    private async Task SendIdentification()
    {
        // The server expects: [PacketType][Identifier string (null-terminated)][OscillatorCount (byte)][Hues (ushort[])][IsLocked (byte)]
        var payload = new List<byte>();
        
        // Packet type
        payload.Add((byte)EDMOPacketType.Identify);
        
        // Identifier string (null-terminated)
        var identBytes = System.Text.Encoding.ASCII.GetBytes(identifier);
        payload.AddRange(identBytes);
        payload.Add(0); // Null terminator
        
        // Oscillator count (1 byte) - NOT ushort!
        payload.Add((byte)oscillatorCount);
        
        // Hues for each oscillator (2 bytes each, ushort, little endian)
        for (int i = 0; i < oscillatorCount; i++)
        {
            ushort hue = (ushort)(360 * i / oscillatorCount);
            payload.Add((byte)(hue & 0xFF));
            payload.Add((byte)((hue >> 8) & 0xFF));
        }
        
        // Lock status byte (0 = unlocked, 1 = locked)
        payload.Add(0);

        await SendPacket(payload.ToArray());
        Console.WriteLine($"📤 Sent identification: '{identifier}' with {oscillatorCount} oscillators");
    }

    private void HandleSessionStart(byte[] packet)
    {
        if (packet.Length >= 5)
        {
            sessionStartOffset = BitConverter.ToUInt32(packet, 1);
            sessionStartTime = DateTime.Now;
            sessionStarted = true;
        }
    }

    private void HandleSessionEnd()
    {
        sessionStarted = false;
        sessionStartTime = DateTime.MinValue;
        sessionStartOffset = 0;
    }

    private async Task SendCurrentTime()
    {
        uint currentTime = GetCurrentTime();
        var payload = new byte[5];
        payload[0] = (byte)EDMOPacketType.GetTime;
        BitConverter.GetBytes(currentTime).CopyTo(payload, 1);
        
        await SendPacket(payload);
    }

    private void HandleOscillatorUpdate(byte[] packet)
    {
        if (packet.Length < 2)
            return;

        byte oscillatorIndex = packet[1];
        
        if (oscillatorIndex >= oscillatorCount)
            return;

        // Parse OscillatorParams (4 floats: frequency, amplitude, offset, phaseShift)
        if (packet.Length >= 18) // 1 (type) + 1 (index) + 16 (4 floats)
        {
            float frequency = BitConverter.ToSingle(packet, 2);
            float amplitude = BitConverter.ToSingle(packet, 6);
            float offset = BitConverter.ToSingle(packet, 10);
            float phaseShift = BitConverter.ToSingle(packet, 14);

            oscillatorStates[oscillatorIndex] = new OscillatorState(
                frequency,
                amplitude,
                offset,
                phaseShift,
                oscillatorStates[oscillatorIndex].Phase
            );

            Console.WriteLine($"🔄 Updated oscillator {oscillatorIndex}: F={frequency:F2}Hz A={amplitude:F2}");
        }
    }

    private async Task SendMotorData()
    {
        for (byte i = 0; i < oscillatorCount; i++)
        {
            var packet = new OscillatorDataPacket(i, oscillatorStates[i]);
            var bytes = StructToBytes(packet);
            await SendPacket(bytes);
        }
    }

    private async Task SendIMUData()
    {
        var packet = new IMUDataPacket(currentIMUData);
        var bytes = StructToBytes(packet);
        await SendPacket(bytes);
    }

    private async Task SendAllData()
    {
        uint currentTime = GetCurrentTime();
        var payload = new List<byte>
        {
            (byte)EDMOPacketType.SendAllData
        };
        payload.AddRange(BitConverter.GetBytes(currentTime));

        // Add all oscillator states
        foreach (var state in oscillatorStates)
        {
            payload.AddRange(StructToBytes(state));
        }

        // Add IMU data
        payload.AddRange(StructToBytes(currentIMUData));

        await SendPacket(payload.ToArray());
    }

    private async Task SendPacket(byte[] payload)
    {
        if (serverEndpoint == null)
            return;

        var escaped = EDMOPacket.EscapePacket(payload);
        var fullPacket = new List<byte>();
        fullPacket.AddRange(EDMOPacket.HEADER);
        fullPacket.AddRange(escaped);
        fullPacket.AddRange(EDMOPacket.FOOTER);

        await udpClient!.SendAsync(fullPacket.ToArray(), serverEndpoint);
    }

    private async Task SimulateRobotBehavior()
    {
        var ct = cancellationTokenSource!.Token;
        
        while (!ct.IsCancellationRequested)
        {
            try
            {
                await Task.Delay(100, ct); // 10 Hz update rate

                if (sessionStarted)
                {
                    // Update oscillator phases based on frequency
                    float deltaTime = 0.1f; // 100ms
                    for (int i = 0; i < oscillatorCount; i++)
                    {
                        var state = oscillatorStates[i];
                        float newPhase = state.Phase + (state.Frequency * 2 * MathF.PI * deltaTime);
                        newPhase = newPhase % (2 * MathF.PI);
                        
                        oscillatorStates[i] = new OscillatorState(
                            state.Frequency,
                            state.Amplitude,
                            state.Offset,
                            state.PhaseShift,
                            newPhase
                        );
                    }

                    // Simulate some IMU movement
                    SimulateIMUData();

                    // Send all data to server
                    await SendAllData();
                }
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ Error in simulation loop: {ex.Message}");
            }
        }
    }

    private void SimulateIMUData()
    {
        uint timestamp = GetCurrentTime();
        
        // Simulate subtle gyroscope movement
        var gyro = new Vector3(
            (float)(random.NextDouble() - 0.5) * 0.1f,
            (float)(random.NextDouble() - 0.5) * 0.1f,
            (float)(random.NextDouble() - 0.5) * 0.1f
        );

        // Simulate accelerometer with gravity + small variations
        var accel = new Vector3(
            (float)(random.NextDouble() - 0.5) * 0.5f,
            (float)(random.NextDouble() - 0.5) * 0.5f,
            9.81f + (float)(random.NextDouble() - 0.5) * 0.2f
        );

        currentIMUData = new IMUData(
            Gyroscope: new SensorInfo<Vector3> { Timestamp = timestamp, Accuracy = 3, Data = gyro },
            Accelerometer: new SensorInfo<Vector3> { Timestamp = timestamp, Accuracy = 3, Data = accel },
            MagneticField: currentIMUData.MagneticField,
            Gravity: new SensorInfo<Vector3> { Timestamp = timestamp, Accuracy = 3, Data = new Vector3(0, 0, 9.81f) },
            Rotation: new SensorInfo<Quaternion> { Timestamp = timestamp, Accuracy = 3, Data = Quaternion.Identity }
        );
    }

    private uint GetCurrentTime()
    {
        if (!sessionStarted)
            return 0;
            
        return (uint)(DateTime.Now - sessionStartTime).TotalMilliseconds + sessionStartOffset;
    }

    private static byte[] StructToBytes<T>(T data) where T : unmanaged
    {
        int size = Marshal.SizeOf(data);
        byte[] bytes = new byte[size];
        IntPtr ptr = Marshal.AllocHGlobal(size);

        try
        {
            Marshal.StructureToPtr(data, ptr, false);
            Marshal.Copy(ptr, bytes, 0, size);
        }
        finally
        {
            Marshal.FreeHGlobal(ptr);
        }

        return bytes;
    }
}
