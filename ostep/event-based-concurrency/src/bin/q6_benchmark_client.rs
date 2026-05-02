use std::io::{Read, Write};
use std::net::TcpStream;
use std::time::Instant;

fn main() -> std::io::Result<()> {
    let addr = std::env::args()
        .nth(1)
        .unwrap_or_else(|| String::from("127.0.0.1:7878"));
    let path = std::env::args()
        .nth(2)
        .unwrap_or_else(|| String::from("README.md"));
    let requests = std::env::args()
        .nth(3)
        .and_then(|value| value.parse::<usize>().ok())
        .unwrap_or(100);

    let start = Instant::now();
    let mut bytes = 0_usize;

    for _ in 0..requests {
        let mut stream = TcpStream::connect(&addr)?;
        stream.write_all(path.as_bytes())?;
        stream.write_all(b"\n")?;
        stream.shutdown(std::net::Shutdown::Write)?;

        let mut response = Vec::new();
        stream.read_to_end(&mut response)?;
        bytes += response.len();
    }

    let elapsed = start.elapsed();
    let seconds = elapsed.as_secs_f64();
    println!("requests: {requests}");
    println!("bytes: {bytes}");
    println!("elapsed_seconds: {:.3}", seconds);
    println!("requests_per_second: {:.1}", requests as f64 / seconds);

    Ok(())
}
