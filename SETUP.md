# EDMO Lab Setup Guide

✅ **Complete mock lab environment running with Docker**

## Quick Start (3 commands)

```bash
# 1. Build (first time only)
docker compose build

# 2. Start everything
docker compose up -d

# 3. Open browser
# Navigate to: http://localhost:8080
```

**That's it!** You should see "Snake1" available for selection.

---

## What's Included

- ✅ **EDMO Server** - Web interface on port 8080
- ✅ **Mock Robot "Snake1"** - 4 oscillators (motors)
- ✅ Real-time control, IMU sensors, multi-user support
- ✅ Session logging and plugin system

---

## Basic Commands

```bash
# Start
docker compose up -d

# Stop
docker compose down

# View logs (real-time)
docker compose logs -f

# Check status
docker compose ps

# Restart
docker compose restart
```

---

## Verify It's Working

After `docker compose up -d`, check:

```bash
docker compose logs edmo | grep "Snake1"
```

You should see: `Opened fused EDMO connection for identifier Snake1`

Then open **http://localhost:8080** and you'll see Snake1 available!

---

## Using the Interface

1. **Select language**: English or Nederlands
2. **Enter your name**: e.g., "TestUser"
3. **Select robot**: Pick "Snake1"
4. **Click "Let's start!"**
5. **Control**: Use sliders to control motors

---

## Multiple Robots

Add a second robot:

```bash
docker compose --profile multi-robot up -d
```

This adds "Snake2" (6 oscillators).

---

## Troubleshooting

### Robot not showing up
```bash
# Wait 10 seconds, then check logs
docker compose logs edmo | grep "Snake1"

# Should see: "Opened fused EDMO connection for identifier Snake1"

# If not, restart
docker compose restart
```

### Clear browser cache
Press `Ctrl+Shift+Delete`, clear cache, reload page.

### Start fresh
```bash
docker compose down
docker compose build
docker compose up -d
```

### View real-time logs
```bash
# All logs
docker compose logs -f

# Server only
docker compose logs -f edmo

# Mock robot only
docker compose logs -f mock-edmo
```

---

## Customize Robots

Edit `docker-compose.yaml`:

```yaml
mock-edmo:
  environment:
    - ROBOT_NAME=Spider1    # Change name
    - OSCILLATOR_COUNT=8    # Change motor count
```

Then rebuild:
```bash
docker compose build mock-edmo
docker compose up -d
```

---

## Architecture

```
Browser (localhost:8080)
    ↓
EDMO Server (edmo container)
    ↓ UDP communication via Docker network
Mock Robot (mock-edmo container)
```

---

## Session Data

Logs are saved in the container:

```bash
# List sessions
docker compose exec edmo ls -la /app/Logs/Sessions

# Copy to your machine
docker cp edmo-edmo-1:/app/Logs ./ServerLogs
```

---

## Add More Robots

Edit `docker-compose.yaml`, add:

```yaml
  my-robot:
    build:
      context: .
      dockerfile: ServerVNext/EDMOMockSimulator/Dockerfile
    environment:
      - ROBOT_NAME=CustomBot
      - OSCILLATOR_COUNT=12
    depends_on:
      - edmo
    networks:
      - edmo-network
```

Start it:
```bash
docker compose up -d my-robot
```

---

## Quick Reference

| What | Command |
|------|---------|
| Start | `docker compose up -d` |
| Stop | `docker compose down` |
| Status | `docker compose ps` |
| Logs | `docker compose logs -f` |
| Restart | `docker compose restart` |
| Rebuild | `docker compose build` |
| Clean start | `docker compose down && docker compose up -d` |
| Add 2nd robot | `docker compose --profile multi-robot up -d` |

---

## Complete Workflow Example

```bash
# 1. First time setup
docker compose build

# 2. Start everything
docker compose up -d

# 3. Verify (wait 10 seconds)
docker compose logs edmo | grep "Snake1"
# Should show: "Opened fused EDMO connection for identifier Snake1"

# 4. Open browser
# http://localhost:8080

# 5. Use the system!
# Select Snake1, enter name, control robot

# 6. When done
docker compose down
```

---

## That's It!

You now have a complete EDMO lab environment running locally. No physical hardware needed! 🤖

**Server**: http://localhost:8080  
**Logs**: `docker compose logs -f`  
**Stop**: `docker compose down`
