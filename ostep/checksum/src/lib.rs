use std::io::{self, Read, Write};

pub const BLOCK_SIZE: usize = 4096;

/// Compute an 8-bit XOR checksum.
///
/// Rules:
/// - Start from 0.
/// - XOR each byte into the running checksum.
/// - Return the final value.
///
/// Fill in the TODOs, then run:
///
/// ```text
/// cargo test
/// ```
#[allow(unused_variables, unused_mut)]
pub fn xor_checksum(data: &[u8]) -> u8 {
    let mut checksum = 0u8;

    for byte in data {
        // TODO 1:
        // Update `checksum` by XOR-ing the current byte into it.
        //
        // Hint:
        // - `byte` has type `&u8`, so dereference it with `*byte`.
        // - XOR assignment in Rust is `^=`.
        checksum ^= *byte;
    }

    // TODO 2:
    // Return the final checksum.
    checksum
}

/// Compute a 16-bit Fletcher checksum.
///
/// Rules:
/// - Keep two running sums: `sum1` and `sum2`.
/// - Start both at 0.
/// - For each byte:
///   - Add the byte to `sum1`, then reduce it modulo 255.
///   - Add `sum1` to `sum2`, then reduce it modulo 255.
/// - Return `sum2` as the high byte and `sum1` as the low byte.
///
/// Fill in the TODOs, then run:
///
/// ```text
/// cargo test
/// ```
#[allow(unused_variables, unused_mut)]
pub fn fletcher16_checksum(data: &[u8]) -> u16 {
    let mut sum1 = 0u16;
    let mut sum2 = 0u16;

    for byte in data {
        // TODO 1:
        // Add the current byte to `sum1`, then take modulo 255.
        //
        // Hint:
        // - `byte` has type `&u8`.
        // - Convert it to u16 with `u16::from(*byte)`.
        sum1 = (sum1 + u16::from(*byte)) % 255;

        // TODO 2:
        // Add the new `sum1` to `sum2`, then take modulo 255.
        sum2 = (sum1 + sum2) % 255;
    }

    // TODO 3:
    // Put `sum2` in the high byte and `sum1` in the low byte.
    (sum2 << 8) | sum1
}

/// Compute a CRC-16/XMODEM checksum.
///
/// Parameters:
/// - width: 16 bits
/// - polynomial: 0x1021
/// - initial value: 0x0000
/// - no input/output reflection
/// - final xor value: 0x0000
pub fn crc16_xmodem(data: &[u8]) -> u16 {
    let mut crc = 0u16;

    for byte in data {
        crc ^= u16::from(*byte) << 8;

        for _ in 0..8 {
            if (crc & 0x8000) != 0 {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    crc
}

pub fn write_block_checksums<R, W>(mut input: R, mut output: W) -> io::Result<usize>
where
    R: Read,
    W: Write,
{
    let mut block = [0u8; BLOCK_SIZE];
    let mut blocks = 0;

    loop {
        let bytes_read = input.read(&mut block)?;
        if bytes_read == 0 {
            break;
        }

        let checksum = xor_checksum(&block[..bytes_read]);
        output.write_all(&[checksum])?;
        blocks += 1;
    }

    output.flush()?;
    Ok(blocks)
}

pub fn verify_block_checksums<R, C>(mut input: R, mut checksums: C) -> io::Result<bool>
where
    R: Read,
    C: Read,
{
    let mut block = [0u8; BLOCK_SIZE];
    let mut expected = [0u8; 1];

    loop {
        let bytes_read = input.read(&mut block)?;
        if bytes_read == 0 {
            return Ok(checksums.read(&mut expected)? == 0);
        }

        if checksums.read_exact(&mut expected).is_err() {
            return Ok(false);
        }

        if xor_checksum(&block[..bytes_read]) != expected[0] {
            return Ok(false);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{
        BLOCK_SIZE, crc16_xmodem, fletcher16_checksum, verify_block_checksums,
        write_block_checksums, xor_checksum,
    };

    #[test]
    fn empty_input_is_zero() {
        assert_eq!(xor_checksum(b""), 0x00);
    }

    #[test]
    fn single_byte_is_itself() {
        assert_eq!(xor_checksum(&[0xab]), 0xab);
    }

    #[test]
    fn xor_multiple_bytes() {
        assert_eq!(xor_checksum(&[0x12, 0x34, 0x56]), 0x70);
    }

    #[test]
    fn equal_values_cancel_out() {
        assert_eq!(xor_checksum(&[0xff, 0xff, 0x42]), 0x42);
    }

    #[test]
    fn text_input() {
        assert_eq!(xor_checksum(b"hello"), 0x62);
    }

    #[test]
    fn fletcher_empty_input_is_zero() {
        assert_eq!(fletcher16_checksum(b""), 0x0000);
    }

    #[test]
    fn fletcher_single_byte() {
        assert_eq!(fletcher16_checksum(&[0xab]), 0xabab);
    }

    #[test]
    fn fletcher_multiple_bytes() {
        assert_eq!(fletcher16_checksum(&[0x12, 0x34, 0x56]), 0xf49c);
    }

    #[test]
    fn fletcher_wraps_modulo_255() {
        assert_eq!(fletcher16_checksum(&[0xff, 0xff, 0x42]), 0x4242);
    }

    #[test]
    fn fletcher_text_input() {
        assert_eq!(fletcher16_checksum(b"hello"), 0x2d16);
    }

    #[test]
    fn crc_empty_input_is_zero() {
        assert_eq!(crc16_xmodem(b""), 0x0000);
    }

    #[test]
    fn crc_standard_check_value() {
        assert_eq!(crc16_xmodem(b"123456789"), 0x31c3);
    }

    #[test]
    fn crc_single_byte() {
        assert_eq!(crc16_xmodem(&[0xab]), 0x0481);
    }

    #[test]
    fn crc_multiple_bytes() {
        assert_eq!(crc16_xmodem(&[0x12, 0x34, 0x56]), 0xde61);
    }

    #[test]
    fn crc_text_input() {
        assert_eq!(crc16_xmodem(b"hello"), 0xc362);
    }

    #[test]
    fn block_checksums_for_empty_input_are_empty() {
        let mut output = Vec::new();
        let blocks = write_block_checksums(&b""[..], &mut output).unwrap();

        assert_eq!(blocks, 0);
        assert_eq!(output, b"");
        assert!(verify_block_checksums(&b""[..], &output[..]).unwrap());
    }

    #[test]
    fn block_checksums_store_one_byte_per_block() {
        let data = vec![0x5a; BLOCK_SIZE + 3];
        let mut output = Vec::new();
        let blocks = write_block_checksums(&data[..], &mut output).unwrap();

        assert_eq!(blocks, 2);
        assert_eq!(output.len(), 2);
        assert_eq!(output[0], 0x00);
        assert_eq!(output[1], 0x5a);
        assert!(verify_block_checksums(&data[..], &output[..]).unwrap());
    }

    #[test]
    fn block_checksum_verification_detects_corruption() {
        let mut data = vec![0u8; BLOCK_SIZE + 10];
        data[20] = 0x11;
        data[BLOCK_SIZE + 2] = 0x22;

        let mut checksums = Vec::new();
        write_block_checksums(&data[..], &mut checksums).unwrap();

        data[BLOCK_SIZE + 2] ^= 0x01;

        assert!(!verify_block_checksums(&data[..], &checksums[..]).unwrap());
    }

    #[test]
    fn block_checksum_verification_rejects_wrong_checksum_count() {
        let data = vec![0x7f; BLOCK_SIZE + 1];
        let mut checksums = Vec::new();
        write_block_checksums(&data[..], &mut checksums).unwrap();

        checksums.pop();

        assert!(!verify_block_checksums(&data[..], &checksums[..]).unwrap());
    }
}
