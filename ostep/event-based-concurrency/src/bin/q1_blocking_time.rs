use std::io::Write;
use std::net::TcpListener;
use std::time::{SystemTime, UNIX_EPOCH};

fn main() -> std::io::Result<()> {
    let addr = std::env::args()
        .nth(1)
        .unwrap_or_else(|| String::from("127.0.0.1:7878"));
    let listener = TcpListener::bind(&addr)?;

    println!("q1 blocking time server listening on {addr}");

    for stream in listener.incoming() {
        let mut stream = stream?;
        stream.write_all(current_time_response().as_bytes())?;
    }

    Ok(())
}

fn current_time_response() -> String {
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("system clock is before unix epoch");
    format!("unix_time_seconds {}\n", now.as_secs())
}
