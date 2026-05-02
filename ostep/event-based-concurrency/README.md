Questions

1. First, write a simple server that can accept and serve TCP connec-
tions. You’ll have to poke around the Internet a bit if you don’t
already know how to do this. Build this to serve exactly one re-
quest at a time; have each request be very simple, e.g., to get the
current time of day.

2. Now, add the select() interface. Build a main program that can
accept multiple connections, and an event loop that checks which
file descriptors have data on them, and then read and process those
requests. Make sure to carefully test that you are using select()
correctly.

3. Next, let’s make the requests a little more interesting, to mimic a
simple web or file server. Each request should be to read the con-
tents of a file (named in the request), and the server should respond
by reading the file into a buffer, and then returning the contents
to the client. Use the standard open(), read(), close() system
calls to implement this feature. Be a little careful here: if you leave
this running for a long time, someone may figure out how to use it
to read all the files on your computer!

4. Now, instead of using standard I/O system calls, use the asyn-
chronous I/O interfaces as described in the chapter. How hard was
it to incorporate asynchronous interfaces into your program?

5. For fun, add some signal handling to your code. One common use
of signals is to poke a server to reload some kind of configuration
file, or take some other kind of administrative action. Perhaps one
natural way to play around with this is to add a user-level file cache
to your server, which stores recently accessed files. Implement a
signal handler that clears the cache when the signal is sent to the
server process.

6. Finally, we have the hard part: how can you tell if the effort to build
an asynchronous, event-based approach are worth it? Can you cre-
ate an experiment to show the benefits? How much implementa-
tion complexity did your approach add?

## Rust start

This directory contains a Rust implementation that follows the exercise
incrementally and uses the `nix` crate for Unix APIs:

- `time-blocking`: accepts one TCP connection at a time and returns the current
  Unix timestamp.
- `time-select`: accepts many TCP connections and uses `select()` to find ready
  sockets.
- `file-select`: uses `select()` for clients, reads the requested relative file
  with `open()`, `read()`, and `close()`, then returns the file bytes.

Run the server:

```sh
cargo run -- --mode file-select --addr 127.0.0.1:7878
```

Request a file from another terminal:

```sh
printf 'README.md\n' | nc 127.0.0.1 7878
```

Try the simpler milestones:

```sh
cargo run -- --mode time-blocking
cargo run -- --mode time-select
```

Or run the question-by-question versions:

```sh
cargo run --bin q1_blocking_time -- 127.0.0.1:7878
cargo run --bin q2_select_time -- 127.0.0.1:7878
cargo run --bin q3_select_file -- 127.0.0.1:7878
cargo run --bin q4_aio_file -- 127.0.0.1:7878
cargo run --bin q5_signal_cache -- 127.0.0.1:7878
```

Use the question 6 benchmark client against one running server:

```sh
cargo run --bin q6_benchmark_client -- 127.0.0.1:7878 README.md 1000
```

The file server keeps a small in-memory cache. Send `SIGHUP` to clear it:

```sh
kill -HUP <server-pid>
```

For safety, the file server only accepts relative paths and rejects absolute
paths or paths containing `..`.
