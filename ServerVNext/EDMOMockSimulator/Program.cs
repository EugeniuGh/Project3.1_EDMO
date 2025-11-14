using EDMOMockSimulator;

Console.WriteLine("╔════════════════════════════════════════════════════════════╗");
Console.WriteLine("║         EDMO Mock Robot Simulator                         ║");
Console.WriteLine("║         Simulates real EDMO hardware for testing          ║");
Console.WriteLine("╚════════════════════════════════════════════════════════════╝");
Console.WriteLine();

// Parse command line arguments
string robotName = "Snake1";
int oscillatorCount = 4;
int udpPort = 2121;
int serialPort = 0; // 0 means UDP only

for (int i = 0; i < args.Length; i++)
{
    switch (args[i])
    {
        case "--name" or "-n" when i + 1 < args.Length:
            robotName = args[++i];
            break;
        case "--oscillators" or "-o" when i + 1 < args.Length:
            oscillatorCount = int.Parse(args[++i]);
            break;
        case "--port" or "-p" when i + 1 < args.Length:
            udpPort = int.Parse(args[++i]);
            break;
        case "--help" or "-h":
            Console.WriteLine("Usage: EDMOMockSimulator [options]");
            Console.WriteLine();
            Console.WriteLine("Options:");
            Console.WriteLine("  --name, -n <name>          Robot identifier (default: Snake1)");
            Console.WriteLine("  --oscillators, -o <count>  Number of oscillators (default: 4)");
            Console.WriteLine("  --port, -p <port>          UDP port (default: 2121)");
            Console.WriteLine("  --help, -h                 Show this help message");
            Console.WriteLine();
            Console.WriteLine("Examples:");
            Console.WriteLine("  EDMOMockSimulator --name Snake2 --oscillators 6");
            Console.WriteLine("  EDMOMockSimulator -n TestBot -o 8 -p 2121");
            return;
    }
}

Console.WriteLine($"Configuration:");
Console.WriteLine($"  Robot Name:       {robotName}");
Console.WriteLine($"  Oscillators:      {oscillatorCount}");
Console.WriteLine($"  UDP Port:         {udpPort}");
Console.WriteLine();

// Create and start the mock robot
var mockRobot = new MockEDMORobot(robotName, oscillatorCount, udpPort);

// Handle graceful shutdown
Console.CancelKeyPress += (sender, e) =>
{
    e.Cancel = true;
    Console.WriteLine("\n🛑 Shutting down gracefully...");
    mockRobot.Stop();
};

mockRobot.Start();

Console.WriteLine("✅ Mock robot is running. Press Ctrl+C to stop.");
Console.WriteLine();

// Keep running until cancelled
await Task.Delay(Timeout.Infinite, mockRobot.CancellationToken);
