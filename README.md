# Multithreaded Web Server

A small multithreaded HTTP web server written in C++ for Unix/Linux environments. The server listens on a TCP port, accepts HTTP `GET` and `HEAD` requests, serves files from a configured root directory, and logs handled requests.

This project is useful for learning how a basic web server works internally: sockets, request parsing, scheduling, worker threads, file serving, and HTTP response generation.

## Features

- Serves static files from a configurable root directory.
- Supports HTTP `GET` requests.
- Supports HTTP `HEAD` requests.
- Uses a scheduler thread and worker threads for concurrent request handling.
- Supports two scheduling policies:
  - `FCFS`: first come, first served.
  - `SJF`: shortest job first, based on file size.
- Can write request logs to a file.
- Can also print logs to the terminal when logging is enabled.
- Sends basic HTTP response headers such as:
  - `Date`
  - `Server`
  - `Last-Modified`
  - `Content-Type`
  - `Content-Length`
  - `Connection`

## Project Structure

```text
.
├── files/                  # Example files served by the server
├── src/
│   ├── makefile            # Build file
│   ├── myhttpd.cpp         # Program entry point and command-line parsing
│   ├── myhttpd.h           # Shared configuration and synchronization declarations
│   ├── myserver.cpp        # Socket setup, bind, listen, and accept loop
│   ├── myserver.h
│   ├── parse.cpp           # Request parsing, scheduling queue, worker queue
│   ├── parse.h
│   ├── senddata.cpp        # HTTP response generation and file sending
│   └── senddata.h
└── README.md
```

## Requirements

This server uses Unix/POSIX networking APIs, so it should be built and run on Linux, WSL, or another Unix-like environment.

Required tools:

- `g++`
- `make`
- POSIX sockets
- C++11 or newer

On Ubuntu/WSL, install the build tools with:

```bash
sudo apt update
sudo apt install build-essential
```

## Build

From the project root:

```bash
cd src
make clean
make
```

This creates the executable:

```text
src/myhttpd
```

## Run

Example:

```bash
./myhttpd -p 8080 -r ../files -n 4 -s FCFS -l server_log.txt
```

Then open this in a browser:

```text
http://127.0.0.1:8080/sample.html
```

Or test with `curl`:

```bash
curl -i http://127.0.0.1:8080/sample.html
curl -I http://127.0.0.1:8080/sample.html
```

## Command-Line Options

```text
myhttpd [-d] [-h] [-l file] [-p port] [-r directory] [-t seconds] [-n thread_count] [-s FCFS|SJF]
```

| Option | Description |
| --- | --- |
| `-d` | Run in debug/single-request mode. |
| `-h` | Show usage help and exit. |
| `-l file` | Enable logging and append logs to the given file. Logs are also printed to the terminal. |
| `-p port` | Port number to listen on. Default: `8080`. |
| `-r directory` | Root directory for served files. Default: `../files`. |
| `-t seconds` | Initial scheduler wait time. Default: `30`. |
| `-n thread_count` | Number of worker threads. Default: `10`, maximum: `30`. |
| `-s FCFS|SJF` | Scheduling policy. Default: `FCFS`. |

## Examples

Run on port `8080` with 4 worker threads:

```bash
./myhttpd -p 8080 -r ../files -n 4 -s FCFS
```

Run with shortest-job-first scheduling:

```bash
./myhttpd -p 8080 -r ../files -n 4 -s SJF
```

Run with logging enabled:

```bash
./myhttpd -p 8080 -r ../files -n 4 -s FCFS -l server_log.txt
```

View the log file:

```bash
cat server_log.txt
```

## How It Works

The server starts by parsing command-line options in `myhttpd.cpp`. It then creates:

- one scheduler thread
- multiple worker threads
- one main accept loop

The main thread listens for incoming TCP connections. For each accepted connection, the request is parsed and placed into a ready queue.

The scheduler thread removes requests from the ready queue using the selected scheduling policy:

- `FCFS`: take the oldest request first.
- `SJF`: sort by file size and serve the smallest request first.

Worker threads wait for scheduled requests. When a worker receives a request, it builds an HTTP response and sends the requested file or an error/directory-listing response.

## Logging

Logging is enabled with `-l`.

Example:

```bash
./myhttpd -p 8080 -r ../files -n 4 -s FCFS -l server_log.txt
```

Each log line includes:

- client IP address
- request time
- service time
- HTTP request line
- response status code
- file size

Example:

```text
127.0.0.1  [06/15/26:17:20:11]  [06/15/26:17:20:12]  GET /sample.html HTTP/1.1 200 343
```

## Supported Content Types

The server detects a small set of common file extensions:

| Extension | Content-Type |
| --- | --- |
| `.html`, `.htm` | `text/html` |
| `.txt`, `.text` | `text/plain` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.jpg`, `.jpeg` | `image/jpeg` |
| `.gif` | `image/gif` |
| `.png` | `image/png` |

Unknown extensions are served as:

```text
application/octet-stream
```

## Clean Build Files

```bash
cd src
make clean
```

## Known Limitations

This is an educational web server, not a production-ready HTTP server.

Current limitations include:

- The networking layer still uses POSIX APIs internally, but socket descriptors are wrapped with a small RAII helper where ownership matters.
- Request parsing is intentionally simple and does not implement the full HTTP specification.
- The server only supports `GET` and `HEAD`.
- Request bodies are not supported.
- Directory listing behavior is basic.
- The main accept loop still reads/parses the request before scheduling it, so a very slow client can delay accepting new clients.
- Path security is improved compared to the original version, but a production server should use canonical path validation and stricter error handling.
- TLS/HTTPS is not supported.

## Quick Test

After starting the server:

```bash
curl -i http://127.0.0.1:8080/sample.html
```

Expected result:

```text
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: ...
```

For a `HEAD` request:

```bash
curl -I http://127.0.0.1:8080/sample.html
```

This should return headers only, with no response body.
