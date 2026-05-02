use nix::sys::select::{select, FdSet};
use std::collections::HashMap;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::os::fd::{AsRawFd, BorrowedFd, RawFd};
use std::time::{SystemTime, UNIX_EPOCH};

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

    println!("q2 select time server listening on {addr}");

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
                    client
                        .stream
                        .write_all(current_time_response().as_bytes())?;
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

fn current_time_response() -> String {
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system clock is before unix epoch");
    format!("unix_time_seconds {}\n", now.as_secs())
}

fn insert_fd(set: &mut FdSet<'_>, fd: RawFd) {
    unsafe {
        set.insert(BorrowedFd::borrow_raw(fd));
    }
}

fn contains_fd(set: &FdSet<'_>, fd: RawFd) -> bool {
    unsafe { set.contains(BorrowedFd::borrow_raw(fd)) }
}
