# ========================
# EDMO Mock Simulator Dockerfile
# .NET 9 runtime for mock robot
# ========================

FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

# Copy project files
COPY ServerVNext/ServerCore/ServerCore.csproj ServerVNext/ServerCore/
COPY ServerVNext/EDMOMockSimulator/EDMOMockSimulator.csproj ServerVNext/EDMOMockSimulator/

# Restore
RUN dotnet restore ServerVNext/EDMOMockSimulator/EDMOMockSimulator.csproj

# Copy source
COPY ServerVNext/ServerCore ServerVNext/ServerCore
COPY ServerVNext/EDMOMockSimulator ServerVNext/EDMOMockSimulator

# Publish
RUN dotnet publish ServerVNext/EDMOMockSimulator/EDMOMockSimulator.csproj -c Release -o /app/publish

# ---------- Runtime ----------
FROM mcr.microsoft.com/dotnet/runtime:9.0 AS final
WORKDIR /app

COPY --from=build /app/publish ./

# Environment variables with defaults
ENV ROBOT_NAME=Snake1
ENV OSCILLATOR_COUNT=4
ENV UDP_PORT=2121

ENTRYPOINT dotnet EDMOMockSimulator.dll --name "$ROBOT_NAME" --oscillators "$OSCILLATOR_COUNT" --port "$UDP_PORT"
