pub fn xor_checksum(data: &[u8]) -> u8 {
    let mut checksum = 0u8;

    for byte in data {
        checksum ^= *byte;
    }

    checksum
}

pub fn fletcher16_checksum(data: &[u8]) -> u16 {
    let mut sum1 = 0u16;
    let mut sum2 = 0u16;

    for byte in data {
        sum1 = (sum1 + u16::from(*byte)) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    (sum2 << 8) | sum1
}

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
