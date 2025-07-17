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

def patch_rust_jitter_percent(rust_types_file):
    """Patch the Rust types.rs file to fix jitter_percent type mismatch"""
    
    if not os.path.exists(rust_types_file):
        log_message(f"Rust types file not found: {rust_types_file}", "ERROR")
        return False
    
    log_message(f"Patching Rust file: {rust_types_file}")
    
    try:
        with open(rust_types_file, 'r') as f:
            content = f.read()
        
        # Check if the file needs patching
        target_pattern = r'jitter_percent:\s*strategy\.jitter_percent,'
        replacement = 'jitter_percent: Some(strategy.jitter_percent),'
        
        if re.search(target_pattern, content):
            create_backup(rust_types_file)
            
            # Apply the patch
            new_content = re.sub(target_pattern, replacement, content)
            
            with open(rust_types_file, 'w') as f:
                f.write(new_content)
            
            log_message(f"Successfully patched jitter_percent in: {rust_types_file}")
            return True
        else:
            # Check if it's already patched
            if 'Some(strategy.jitter_percent)' in content:
                log_message(f"File already patched: {rust_types_file}")
                return True
            else:
                log_message(f"Could not find target pattern in: {rust_types_file}", "WARN")
                log_message("Looking for pattern: jitter_percent: strategy.jitter_percent,", "DEBUG")
                return False
    
    except Exception as e:
        log_message(f"Error patching {rust_types_file}: {e}", "ERROR")
        return False

def verify_rust_patch(rust_types_file):
    """Verify that the Rust patch was applied correctly"""
    try:
        with open(rust_types_file, 'r') as f:
            content = f.read()
        
        # Check for the fixed pattern
        if 'Some(strategy.jitter_percent)' in content:
            log_message("Rust patch verification: SUCCESS")
            return True
        else:
            log_message("Rust patch verification: FAILED", "ERROR")
            return False
    except Exception as e:
        log_message(f"Error verifying patch: {e}", "ERROR")
        return False

def main():
    """Main function to run all patching operations"""
    log_message("Starting build-time patching process")
    
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
        if not patch_rust_jitter_percent(str(rust_types_file)):
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
        log_message("=== Build-time patching completed successfully ===")
        return 0
    else:
        log_message("=== Build-time patching completed with errors ===", "ERROR")
        return 1

# Usage
if __name__ == "__main__":
    sys.exit(main())
