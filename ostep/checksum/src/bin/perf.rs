use checksum::{crc16_xmodem, fletcher16_checksum, xor_checksum};
use std::env;
use std::hint::black_box;
use std::time::{Duration, Instant};

fn main() {
    let size = parse_arg(1).unwrap_or(16 * 1024 * 1024);
    let iterations = parse_arg(2).unwrap_or(100);
    let data = make_data(size);

    println!("data size: {} bytes", data.len());
    println!("iterations: {iterations}");
    println!();

    let xor = bench("xor", iterations, || {
        u64::from(xor_checksum(black_box(&data)))
    });
    let fletcher = bench("fletcher-16", iterations, || {
        u64::from(fletcher16_checksum(black_box(&data)))
    });
    let crc = bench("crc-16/xmodem", iterations, || {
        u64::from(crc16_xmodem(black_box(&data)))
    });

    println!();
    print_result("xor", size, iterations, xor);
    print_result("fletcher-16", size, iterations, fletcher);
    print_result("crc-16/xmodem", size, iterations, crc);
}

fn parse_arg(position: usize) -> Option<usize> {
    env::args().nth(position)?.parse().ok()
}

fn make_data(size: usize) -> Vec<u8> {
    let mut value = 0x1234_5678u32;
    let mut data = Vec::with_capacity(size);

    for _ in 0..size {
        value = value.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
        data.push((value >> 24) as u8);
    }

    data
}

fn bench<F>(name: &str, iterations: usize, mut checksum: F) -> Duration
where
    F: FnMut() -> u64,
{
    let start = Instant::now();
    let mut result = 0u64;

    for _ in 0..iterations {
        result ^= checksum();
    }

    let elapsed = start.elapsed();
    println!("{name} result guard: 0x{result:x}");
    elapsed
}

fn print_result(name: &str, size: usize, iterations: usize, elapsed: Duration) {
    let bytes = size.saturating_mul(iterations);
    let seconds = elapsed.as_secs_f64();
    let mib = bytes as f64 / (1024.0 * 1024.0);
    let throughput = mib / seconds;

    println!(
        "{name}: {:.3} ms total, {:.2} MiB/s",
        seconds * 1000.0,
        throughput
    );
}
