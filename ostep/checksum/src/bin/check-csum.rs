use checksum::verify_block_checksums;
use std::env;
use std::fs::File;
use std::io;
use std::process::ExitCode;

fn main() -> ExitCode {
    match run() {
        Ok(true) => {
            println!("file is OK");
            ExitCode::SUCCESS
        }
        Ok(false) => {
            println!("file has been corrupted");
            ExitCode::from(1)
        }
        Err(message) => {
            eprintln!("{message}");
            ExitCode::FAILURE
        }
    }
}

fn run() -> Result<bool, String> {
    let mut args = env::args();
    let program = args.next().unwrap_or_else(|| "check-csum".to_string());
    let input_path = args
        .next()
        .ok_or_else(|| format!("usage: {program} <input-file> <checksum-file>"))?;
    let checksum_path = args
        .next()
        .ok_or_else(|| format!("usage: {program} <input-file> <checksum-file>"))?;

    if args.next().is_some() {
        return Err(format!("usage: {program} <input-file> <checksum-file>"));
    }

    let input = File::open(&input_path)
        .map_err(|err| format!("failed to open input file {input_path:?}: {err}"))?;
    let checksums = File::open(&checksum_path)
        .map_err(|err| format!("failed to open checksum file {checksum_path:?}: {err}"))?;

    verify_block_checksums(input, checksums).map_err(format_io_error)
}

fn format_io_error(err: io::Error) -> String {
    format!("checksum verification failed: {err}")
}
