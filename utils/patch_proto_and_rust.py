import os
import re
import sys
from pathlib import Path

def log_message(message, level="INFO"):
    """Simple logging function"""
    print(f"[{level}] {message}")

def create_backup(filepath):
    """Create a backup of the file before modifying"""
    backup_path = f"{filepath}.backup"
    if not os.path.exists(backup_path):
        try:
            with open(filepath, 'r') as src, open(backup_path, 'w') as dst:
                dst.write(src.read())
            log_message(f"Created backup: {backup_path}")
        except Exception as e:
            log_message(f"Warning: Could not create backup for {filepath}: {e}", "WARN")

def remove_optional_from_proto(directory):
    """Remove 'optional' keyword from protobuf files"""
    proto_file_pattern = re.compile(r'.*\.proto$')
    files_modified = 0

    log_message(f"Scanning protobuf files in: {directory}")
    
    for root, _, files in os.walk(directory):
        for file in files:
            if proto_file_pattern.match(file):
                filepath = os.path.join(root, file)
                
                try:
                    with open(filepath, 'r') as f:
                        content = f.read()
                    
                    # Check if file needs modification
                    if 'optional ' in content:
                        create_backup(filepath)
                        new_content = re.sub(r'\boptional\s+', '', content)
                        
                        with open(filepath, 'w') as f:
                            f.write(new_content)
                        
                        log_message(f"Updated protobuf: {filepath}")
                        files_modified += 1
                    else:
                        log_message(f"No changes needed: {filepath}")
                        
                except Exception as e:
                    log_message(f"Error processing {filepath}: {e}", "ERROR")
                    return False
    
    log_message(f"Modified {files_modified} protobuf files")
    return True

def patch_rust_types_rs(rust_types_file):
    """Patch the Rust types.rs file to fix type mismatches between protobuf and Rust code.
    
    Fixes several fields that changed from Option<T> to T in protobuf:
    - tcp_nodelay: Option<bool> -> bool
    - pubsub_reconciliation_interval_ms: Option<u32> -> u32
    - jitter_percent: Option<u32> -> u32
    - refresh_interval_seconds: Option<u32> -> u32
    - compression_level: Option<i32> -> i32
    """
    
    if not os.path.exists(rust_types_file):
        log_message(f"Rust types file not found: {rust_types_file}", "ERROR")
        return False
    
    log_message(f"Patching Rust file: {rust_types_file}")
    
    try:
        with open(rust_types_file, 'r') as f:
            content = f.read()
        
        needs_patching = False
        new_content = content
        
        # Fix tcp_nodelay: changed from Option<bool> to bool
        tcp_nodelay_pattern = r'let tcp_nodelay = value\.tcp_nodelay\.unwrap_or\(true\);'
        tcp_nodelay_replacement = 'let tcp_nodelay = value.tcp_nodelay;'
        if re.search(tcp_nodelay_pattern, content):
            create_backup(rust_types_file)
            new_content = re.sub(tcp_nodelay_pattern, tcp_nodelay_replacement, new_content)
            needs_patching = True
            log_message("Applied tcp_nodelay patch")
        
        # Fix pubsub_reconciliation_interval_ms: changed from Option<u32> to u32
        pubsub_pattern = r'value\.pubsub_reconciliation_interval_ms\.filter\(\|&v\| v != 0\);'
        pubsub_replacement = 'if value.pubsub_reconciliation_interval_ms != 0 { Some(value.pubsub_reconciliation_interval_ms) } else { None };'
        if re.search(pubsub_pattern, new_content):
            if not needs_patching:
                create_backup(rust_types_file)
            new_content = re.sub(pubsub_pattern, pubsub_replacement, new_content)
            needs_patching = True
            log_message("Applied pubsub_reconciliation_interval_ms patch")
        
        # Fix jitter_percent: changed from Option<u32> to u32
        jitter_pattern = r'jitter_percent:\s*strategy\.jitter_percent,'
        jitter_replacement = 'jitter_percent: Some(strategy.jitter_percent),'
        if re.search(jitter_pattern, new_content):
            if not needs_patching:
                create_backup(rust_types_file)
            new_content = re.sub(jitter_pattern, jitter_replacement, new_content)
            needs_patching = True
            log_message("Applied jitter_percent patch")
        
        # Fix refresh_interval_seconds: changed from Option<u32> to u32
        refresh_pattern = r'refresh_interval_seconds,\s*\n\s*}'
        refresh_replacement = 'refresh_interval_seconds: Some(refresh_interval_seconds),\n                }'
        if re.search(refresh_pattern, new_content):
            if not needs_patching:
                create_backup(rust_types_file)
            new_content = re.sub(refresh_pattern, refresh_replacement, new_content)
            needs_patching = True
            log_message("Applied refresh_interval_seconds patch")
        
        # Fix compression_level: changed from Option<i32> to i32
        compression_pattern = r'compression_level:\s*proto_config\.compression_level,'
        compression_replacement = 'compression_level: Some(proto_config.compression_level),'
        if re.search(compression_pattern, new_content):
            if not needs_patching:
                create_backup(rust_types_file)
            new_content = re.sub(compression_pattern, compression_replacement, new_content)
            needs_patching = True
            log_message("Applied compression_level patch")
        
        if needs_patching:
            with open(rust_types_file, 'w') as f:
                f.write(new_content)
            log_message(f"Successfully patched Rust types file: {rust_types_file}")
            return True
        else:
            log_message(f"File already patched: {rust_types_file}")
            return True
    
    except Exception as e:
        log_message(f"Error patching {rust_types_file}: {e}", "ERROR")
        return False

def verify_rust_patch(rust_types_file):
    """Verify that the Rust patch was applied correctly"""
    try:
        with open(rust_types_file, 'r') as f:
            content = f.read()
        
        # Check for the fixed patterns
        tcp_nodelay_fixed = 'let tcp_nodelay = value.tcp_nodelay;' in content
        pubsub_fixed = 'if value.pubsub_reconciliation_interval_ms != 0 { Some(value.pubsub_reconciliation_interval_ms) } else { None }' in content
        jitter_fixed = 'Some(strategy.jitter_percent)' in content
        refresh_fixed = 'refresh_interval_seconds: Some(refresh_interval_seconds)' in content
        compression_fixed = 'Some(proto_config.compression_level)' in content
        
        if tcp_nodelay_fixed and pubsub_fixed and jitter_fixed and refresh_fixed and compression_fixed:
            log_message("Rust patch verification: SUCCESS")
            return True
        else:
            missing = []
            if not tcp_nodelay_fixed:
                missing.append("tcp_nodelay fix")
            if not pubsub_fixed:
                missing.append("pubsub_reconciliation_interval_ms fix")
            if not jitter_fixed:
                missing.append("jitter_percent fix")
            if not refresh_fixed:
                missing.append("refresh_interval_seconds fix")
            if not compression_fixed:
                missing.append("compression_level fix")
            log_message(f"Rust patch verification: FAILED - Missing: {', '.join(missing)}", "ERROR")
            return False
    
    except Exception as e:
        log_message(f"Error verifying patch: {e}", "ERROR")
        return False

def main():
    """Main function to run all patching operations.
    
    This script patches protobuf files and Rust code to fix type mismatches
    that occur when protobuf 'optional' fields are removed, causing them to
    change from Option<T> to T in the generated Rust code.
    """
    log_message("Starting build-time patching process for protobuf and Rust files")
    
    # Base directory (relative to script location)
    base_dir = Path(__file__).parent.parent
    
    # Define paths
    protobuf_directory = base_dir / "valkey-glide" / "glide-core" / "src" / "protobuf"
    rust_types_file = base_dir / "valkey-glide" / "glide-core" / "src" / "client" / "types.rs"
    
    success = True
    
    # Step 1: Remove optional from protobuf files
    log_message("=== Phase 1: Patching Protobuf Files ===")
    if protobuf_directory.exists():
        if not remove_optional_from_proto(str(protobuf_directory)):
            success = False
    else:
        log_message(f"Protobuf directory not found: {protobuf_directory}", "ERROR")
        success = False
    
    # Step 2: Patch Rust jitter_percent issue
    log_message("=== Phase 2: Patching Rust Code ===")
    if rust_types_file.exists():
        if not patch_rust_types_rs(str(rust_types_file)):
            success = False
        else:
            # Verify the patch
            if not verify_rust_patch(str(rust_types_file)):
                success = False
    else:
        log_message(f"Rust types file not found: {rust_types_file}", "ERROR")
        success = False
    
    # Summary
    if success:
        log_message("=== Build-time patching for protobuf and Rust files completed successfully ===")
        return 0
    else:
        log_message("=== Build-time patching for protobuf and Rust files completed with errors ===", "ERROR")
        return 1

# Usage
if __name__ == "__main__":
    sys.exit(main())
