use nix::sys::aio::{Aio, AioRead};
use nix::sys::select::{select, FdSet};
use nix::sys::signal::SigevNotify;
use std::collections::HashMap;
use std::fs::File;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::os::fd::{AsFd, AsRawFd, BorrowedFd, RawFd};
use std::thread;
use std::time::Duration;

const MAX_FILE_BYTES: usize = 1024 * 1024;

struct Client {
    stream: TcpStream,
    request: Vec<u8>,
}

fn main() -> std::io::Result<()> {
    let addr = std::env::args()
        .nth(1)
        .unwrap_or_else(|| String::from("127.0.0.1:7878"));
    let listener = TcpListener::bind(&addr)?;
    listener.set_nonblocking(true)?;

    println!("q4 aio file server listening on {addr}");

    let mut clients: HashMap<RawFd, Client> = HashMap::new();

    loop {
        let listener_fd = listener.as_raw_fd();
        let mut readfds = FdSet::new();
        insert_fd(&mut readfds, listener_fd);

        let mut max_fd = listener_fd;
        for fd in clients.keys().copied() {
            insert_fd(&mut readfds, fd);
            max_fd = max_fd.max(fd);
        }

        match select(max_fd + 1, Some(&mut readfds), None, None, None) {
            Ok(_) => {}
            Err(nix::errno::Errno::EINTR) => continue,
            Err(err) => return Err(std::io::Error::from_raw_os_error(err as i32)),
        }

        if contains_fd(&readfds, listener_fd) {
            loop {
                match listener.accept() {
                    Ok((stream, _)) => {
                        stream.set_nonblocking(true)?;
                        clients.insert(
                            stream.as_raw_fd(),
                            Client {
                                stream,
                                request: Vec::new(),
                            },
                        );
                    }
                    Err(err) if err.kind() == std::io::ErrorKind::WouldBlock => break,
                    Err(err) => return Err(err),
                }
            }
        }

        let ready_clients: Vec<RawFd> = clients
            .keys()
            .copied()
            .filter(|fd| contains_fd(&readfds, *fd))
            .collect();

        for fd in ready_clients {
            let finished = if let Some(client) = clients.get_mut(&fd) {
                read_until_newline(client)?
            } else {
                false
            };

            if finished {
                if let Some(mut client) = clients.remove(&fd) {
                    let response = file_response(&client.request);
                    client.stream.write_all(&response)?;
                }
            }
        }
    }
}

fn read_until_newline(client: &mut Client) -> std::io::Result<bool> {
    let mut buf = [0_u8; 1024];

    loop {
        match client.stream.read(&mut buf) {
            Ok(0) => return Ok(true),
            Ok(n) => {
                client.request.extend_from_slice(&buf[..n]);
                if client.request.contains(&b'\n') {
                    return Ok(true);
                }
            }
            Err(err) if err.kind() == std::io::ErrorKind::WouldBlock => return Ok(false),
            Err(err) => return Err(err),
        }
    }
}

fn file_response(request: &[u8]) -> Vec<u8> {
    let path = match parse_path(request) {
        Ok(path) => path,
        Err(message) => return format!("ERR {message}\n").into_bytes(),
    };

    match read_file_with_aio(&path) {
        Ok(contents) => {
            let mut response = format!("OK {} bytes\n", contents.len()).into_bytes();
            response.extend_from_slice(&contents);
            response
        }
        Err(err) => format!("ERR {err}\n").into_bytes(),
    }
}

fn parse_path(request: &[u8]) -> Result<String, &'static str> {
    let line = request
        .split(|byte| *byte == b'\n')
        .next()
        .ok_or("empty request")?;
    let path = std::str::from_utf8(line)
        .map_err(|_| "request path must be utf-8")?
        .trim();

    if path.is_empty() {
        return Err("request path is empty");
    }

    if path.starts_with('/') || path.contains("..") {
        return Err("only relative paths below the current directory are allowed");
    }

    Ok(path.to_string())
}

fn read_file_with_aio(path: &str) -> std::io::Result<Vec<u8>> {
    let file = File::open(path)?;
    let file_len = file.metadata()?.len() as usize;
    let read_len = file_len.min(MAX_FILE_BYTES);
    let mut buffer = vec![0_u8; read_len];

    let mut request = Box::pin(AioRead::new(
        file.as_fd(),
        0,
        &mut buffer,
        0,
        SigevNotify::SigevNone,
    ));
    request.as_mut().submit().map_err(nix_to_io)?;

    loop {
        match request.as_mut().error() {
            Ok(()) => break,
            Err(nix::errno::Errno::EINPROGRESS) => thread::sleep(Duration::from_millis(1)),
            Err(err) => return Err(nix_to_io(err)),
        }
    }

    let bytes_read = request.as_mut().aio_return().map_err(nix_to_io)?;
    drop(request);
    buffer.truncate(bytes_read);
    Ok(buffer)
}

fn insert_fd(set: &mut FdSet<'_>, fd: RawFd) {
    unsafe {
        set.insert(BorrowedFd::borrow_raw(fd));
    }
}

fn contains_fd(set: &FdSet<'_>, fd: RawFd) -> bool {
    unsafe { set.contains(BorrowedFd::borrow_raw(fd)) }
}

fn nix_to_io(err: nix::errno::Errno) -> std::io::Error {
    std::io::Error::from_raw_os_error(err as i32)
}
