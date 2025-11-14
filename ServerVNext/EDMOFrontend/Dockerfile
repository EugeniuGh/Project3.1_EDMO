# ========================
# EDMO Frontend Dockerfile
# .NET 8 LTS + Node 20 for any npm steps
# Place this file at: ServerVNext/EDMOFrontend/Dockerfile
# Build context: repository root (the folder that contains ServerVNext/)
# ========================

FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

# Install Node.js 20 (needed if your csproj runs npm for bundling)
RUN apt-get update && apt-get install -y curl gnupg \
    && curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && node --version && npm --version

# Copy project files first (better layer caching)
COPY ServerVNext/ServerCore/ServerCore.csproj ServerVNext/ServerCore/
COPY ServerVNext/EDMOFrontend/EDMOFrontend.csproj ServerVNext/EDMOFrontend/

# Restore
RUN dotnet restore ServerVNext/EDMOFrontend/EDMOFrontend.csproj

# Copy the rest of the sources
COPY ServerVNext/ServerCore ServerVNext/ServerCore
COPY ServerVNext/EDMOFrontend ServerVNext/EDMOFrontend

# Publish
RUN dotnet publish ServerVNext/EDMOFrontend/EDMOFrontend.csproj -c Release -o /app/publish

# ---------- Runtime ----------
FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS final
WORKDIR /app

COPY --from=build /app/publish ./

ENV ASPNETCORE_URLS=http://0.0.0.0:8080
EXPOSE 8080

# Optional plugins mount
RUN mkdir -p /app/Plugins
VOLUME ["/app/Plugins"]

ENTRYPOINT ["dotnet", "EDMOFrontend.dll"]
