# Ice Hockey Game Server — C++ / Azure

A multiplayer **Ice Hockey game server implemented in C++ using Boost.Asio**, designed to handle client connections and real-time match communication over TCP and UDP.

The server is containerized with Docker and deployed to **Microsoft Azure**, providing a cloud-hosted networking backend for multiplayer game clients.

The architecture separates reliable connection/session operations from latency-sensitive real-time match events:

* **TCP** — client connection, server session management, and create/join operations
* **UDP** — real-time match events and gameplay communication
* **Boost.Asio** — asynchronous networking
* **Docker** — containerization
* **Microsoft Azure** — cloud deployment

## Architecture

```text
                         ┌─────────────────────┐
                         │     Game Client     │
                         └──────────┬──────────┘
                                    │
                       ┌────────────┴────────────┐
                       │                         │
                    TCP │                         │ UDP
                       │                         │
                       ▼                         ▼
              ┌────────────────┐       ┌────────────────┐
              │ Connection /   │       │ Match Events / │
              │ Create / Join  │       │ Real-time Data │
              └───────┬────────┘       └───────┬────────┘
                      │                        │
                      └───────────┬────────────┘
                                  ▼
                       ┌─────────────────────┐
                       │   C++ Game Server   │
                       │     Boost.Asio      │
                       └──────────┬──────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │ Microsoft Azure  │
                         └──────────────────┘
```

## Networking Model

The server uses two transport protocols for different responsibilities.

### TCP — Reliable Operations

TCP is used for operations where reliable and ordered delivery is required.

Current TCP functionality includes:

* Establishing client connections
* Creating a game/match
* Joining an existing game/match
* Connection and session management

TCP provides reliable, ordered communication for these operations.

### UDP — Match Events

UDP is used for **real-time match events**, where low latency is more important than guaranteed delivery.

This channel is intended for time-sensitive game communication where continuously sending current state/events is preferable to waiting for retransmission of an older packet.

The separation between TCP and UDP allows the server to use the appropriate transport mechanism for each type of communication.

## 🛠️ Technologies

| Technology          | Purpose                             |
| ------------------- | ----------------------------------- |
| **C++**             | Core server implementation          |
| **Boost.Asio**      | TCP/UDP networking                  |
| **TCP**             | Connections, create/join operations |
| **UDP**             | Real-time match events              |
| **CMake**           | Build system                        |
| **Docker**          | Containerization                    |
| **Ubuntu Linux**    | Server environment                  |
| **Microsoft Azure** | Cloud deployment                    |

## ☁️ Azure Deployment

The server runs as a Dockerized C++ application deployed to **Microsoft Azure**.

The deployment provides a publicly accessible network endpoint for game clients and demonstrates the complete workflow from local C++ development to cloud-hosted multiplayer networking.

```text
C++ Development
       │
       ▼
     CMake
       │
       ▼
 Docker Image
       │
       ▼
   Azure Host
       │
       ├──────── TCP ────────► Client Connections
       │
       └──────── UDP ────────► Match Events
```

## 🐳 Docker

The server is packaged as a Docker container to provide a consistent Linux runtime environment between development and deployment.

### Build

```bash
docker build -t ice-hockey-server .
```

### Run

The server requires both TCP and UDP networking.

```bash
docker run \
  -p 8080:8080/tcp \
  -p 8081:8081/udp \
  ice-hockey-server
```

> The ports above should be replaced with the actual TCP and UDP ports configured by the server.

## 📁 Project Structure

```text
.
├── src/
│   ├── ...
├── CMakeLists.txt
├── Dockerfile
├── compose.yml
└── README.md
```

## 🎯 Engineering Focus

This project focuses on practical **network programming and multiplayer server development in C++**.

Key areas demonstrated by the project include:

* C++ network programming
* TCP socket communication
* UDP socket communication
* Asynchronous I/O with Boost.Asio
* Client connection management
* Match creation and joining
* Real-time multiplayer event handling
* Separation of reliable and low-latency network traffic
* CMake-based C++ builds
* Linux server development
* Docker containerization
* Cloud deployment on Microsoft Azure

## 🔭 Future Development

Potential future improvements include:

* Expand match and lobby functionality
* Improve client/session lifecycle management
* Add server-side validation
* Add connection timeouts and recovery
* Implement more comprehensive match state management
* Add structured server logging
* Add automated networking tests
* Add CI/CD with GitHub Actions
* Add monitoring and telemetry for the Azure deployment
* Improve scalability for multiple simultaneous matches

## 🤝 Contributing

Feedback and contributions are welcome.

Please open an issue to discuss significant changes before submitting a pull request.

## 📄 License

This project is licensed under the [MIT License](LICENSE).
