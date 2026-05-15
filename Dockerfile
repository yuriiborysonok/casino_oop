# STAGE 1: Build Environment
FROM ubuntu:22.04 AS builder

# Set non-interactive to avoid timezone prompts during apt-get install
ENV DEBIAN_FRONTEND=noninteractive

# Set locale for CMake FetchContent extraction to prevent UTF-8 errors
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
ENV LANGUAGE=C.UTF-8

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpq-dev \
    libpqxx-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the source code
COPY CMakeLists.txt .
COPY include/ include/
COPY src/ src/
COPY tests/ tests/

# Build the project and run tests
RUN mkdir build && cd build && cmake .. && make && ./casino_tests

# STAGE 2: Runtime Environment
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libpq5 \
    libpqxx-6.4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Ensure libpqxx fallback if 6.4 is not available on 22.04
RUN apt-get update && apt-get install -y libpqxx-dev || true

WORKDIR /app

# Copy the compiled executable from the builder stage
COPY --from=builder /app/build/casino_server .

# Copy the frontend files so the server can serve them
COPY frontend/ frontend/

# Expose port 8080 for Cloud Run
EXPOSE 8080

# Command to run the application
CMD ["./casino_server"]
