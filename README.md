# webserv

Hey there! This is my take on building an HTTP server from scratch in C++. It's been a fun challenge to dive deep into networking, sockets, and how the web really works under the hood. No fancy frameworks here—just pure C++ and some good old POSIX calls.

## What This Server Does

I built this to handle basic HTTP requests like GET, POST, and DELETE. It's got multiplexing with `poll()` for handling multiple clients at once without threads, which keeps things efficient. Right now, it's focused on the core HTTP stuff—parsing requests, serving files, and sending responses. CGI and advanced features are on the todo list.

### Key Features
- **Single-threaded multiplexing**: Uses `poll()` to manage multiple connections without blocking.
- **HTTP/1.0 and HTTP/1.1 support**: Handles different versions gracefully.
- **File serving**: Serves static files from a configured root directory.
- **Basic security**: Path traversal protection and configurable limits.
- **Configurable**: Easy to tweak ports, timeouts, and more via a config file.
- **No external deps**: Just standard C++ libraries and system calls.

## Building and Running

### Prerequisites
- A C++ compiler (I use g++ or clang++)
- macOS/Linux (tested on macOS, should work on Linux too)

### Build
```bash
make
```
That should compile everything into a `webserv` binary.

### Run
```bash
./webserv [config_file]
```
If you don't specify a config file, it defaults to `./config/server.conf`. Check the config for port settings and stuff.

### Test It Out
Fire up the server and hit it with curl:
```bash
curl http://localhost:8080/
```
You should get back the index page or a 404 if there's no index.html.

For multiple clients, try my test script:
```bash
./spawnTest
```
It spawns a bunch of connections to stress-test the multiplexing.

## Configuration

The server reads from a config file (inspired by nginx format). Here's a sample:

```
port 8080
root ./www
index index.html index.htm
client_timeout 10
client_max_body_size 1048576
```

- `port`: Which port to listen on
- `root`: Root directory for serving files
- `index`: Index files to look for in directories
- `client_timeout`: How long to wait before kicking inactive clients
- `client_max_body_size`: Max size for request bodies

## How the Multiplexing Works

I started with `select()` but switched to `poll()` for better scalability. The idea is to let the kernel tell us when sockets have data ready, so we don't waste CPU spinning.

Here's the rough flow:
1. Set up pollfd array with server socket and all client sockets
2. Call `poll()` with a timeout
3. When it returns, check which fds are ready
4. Handle new connections or read data from clients
5. Process requests and send responses
6. Repeat

It's efficient because the process sleeps until there's actual work to do.

## Contributing

Got ideas or found a bug? Feel free to open an issue or PR. I'm learning as I go, so any feedback is welcome. Just keep it respectful and helpful.

## License

This is MIT licensed—do whatever you want with it, just don't blame me if it breaks your stuff. 😄

---

Built with ❤️ and a lot of `man` pages.

