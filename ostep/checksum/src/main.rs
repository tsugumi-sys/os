use checksum::{crc16_xmodem, fletcher16_checksum, xor_checksum};
use std::env;

fn main() {
    let input = env::args().skip(1).collect::<Vec<_>>().join(" ");
    let bytes = input.as_bytes();
    let xor = xor_checksum(bytes);
    let fletcher16 = fletcher16_checksum(bytes);
    let crc16 = crc16_xmodem(bytes);

    println!("input: {:?}", input);
    println!("xor checksum: 0x{xor:02x}");
    println!("fletcher-16 checksum: 0x{fletcher16:04x}");
    println!("crc-16/xmodem checksum: 0x{crc16:04x}");
}
