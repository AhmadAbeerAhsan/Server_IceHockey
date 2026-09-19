# Use the official Ubuntu image as the base image
FROM ubuntu:latest AS build

# Install necessary dependencies
# add libcpprest-dev libssl-dev for rest later
RUN apt-get update && apt-get install -y \
    build-essential \
    libboost-dev \
    cmake
RUN g++ --version

# Set the working directory in the container
WORKDIR /app
# Copy the source code into the container
COPY CMakeLists.txt .
COPY src/ ./src/

# Compile the C++ code
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release

# Stage 2: Runtime
FROM ubuntu:latest

WORKDIR /app
COPY --from=build /app/build/ok_api .

# Expose the port on which the API will listen
EXPOSE 8081/tcp
EXPOSE 8080/udp

# Command to run the API when the container starts
ENTRYPOINT ["./ok_api"]
CMD ["8081", "8080"]