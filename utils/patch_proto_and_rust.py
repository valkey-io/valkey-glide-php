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
    """Patch the Rust types.rs file to fix jitter_percent and refresh_interval_seconds type mismatches"""
    
    if not os.path.exists(rust_types_file):
        log_message(f"Rust types file not found: {rust_types_file}", "ERROR")
        return False
    
    log_message(f"Patching Rust file: {rust_types_file}")
    
    try:
        with open(rust_types_file, 'r') as f:
            content = f.read()
        
        # Check if the file needs patching
        jitter_pattern = r'jitter_percent:\s*strategy\.jitter_percent,'
        jitter_replacement = 'jitter_percent: Some(strategy.jitter_percent),'
        
        refresh_pattern = r'refresh_interval_seconds,\s*\n\s*}'
        refresh_replacement = 'refresh_interval_seconds: Some(refresh_interval_seconds),\n                }'
        
        needs_patching = False
        new_content = content
        
        # Apply jitter_percent patch
        if re.search(jitter_pattern, content):
            create_backup(rust_types_file)
            new_content = re.sub(jitter_pattern, jitter_replacement, new_content)
            needs_patching = True
            log_message("Applied jitter_percent patch")
        elif 'Some(strategy.jitter_percent)' in content:
            log_message("jitter_percent already patched")
        
        # Apply refresh_interval_seconds patch
        if re.search(refresh_pattern, new_content):
            if not needs_patching:
                create_backup(rust_types_file)
            new_content = re.sub(refresh_pattern, refresh_replacement, new_content)
            needs_patching = True
            log_message("Applied refresh_interval_seconds patch")
        elif 'refresh_interval_seconds: Some(refresh_interval_seconds)' in new_content:
            log_message("refresh_interval_seconds already patched")
        
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

def patch_request_type_rs(request_type_file):
    """Patch the Rust request_type.rs file to add missing pub/sub RequestType mappings"""
    
    if not os.path.exists(request_type_file):
        log_message(f"Request type file not found: {request_type_file}", "ERROR")
        return False
    
    log_message(f"Patching RequestType file: {request_type_file}")
    
    try:
        with open(request_type_file, 'r') as f:
            content = f.read()
        
        needs_patching = False
        new_content = content
        
        # Patch get_command method - add pub/sub commands before the _ => todo!() line
        get_command_pattern = r'(\s+RequestType::FtSearch => Some\(cmd\("FT\.SEARCH"\)\),)\s*\n(\s+_ => todo!\(\),)'
        get_command_replacement = r'''\1
            RequestType::Subscribe => Some(cmd("SUBSCRIBE")),
            RequestType::PSubscribe => Some(cmd("PSUBSCRIBE")),
            RequestType::Unsubscribe => Some(cmd("UNSUBSCRIBE")),
            RequestType::PUnsubscribe => Some(cmd("PUNSUBSCRIBE")),
            RequestType::SSubscribe => Some(cmd("SSUBSCRIBE")),
            RequestType::SUnsubscribe => Some(cmd("SUNSUBSCRIBE")),
\2'''
        
        if re.search(get_command_pattern, content) and 'RequestType::Subscribe =>' not in content:
            create_backup(request_type_file)
            new_content = re.sub(get_command_pattern, get_command_replacement, new_content)
            needs_patching = True
            log_message("Applied get_command pub/sub patch")
        elif 'RequestType::Subscribe =>' in content:
            log_message("get_command already patched")
        
        # Patch From<ProtobufRequestType> trait - add pub/sub mappings before the _ => todo!() line
        from_trait_pattern = r'(\s+ProtobufRequestType::FtSearch => RequestType::FtSearch,)\s*\n(\s+_ => todo!\(\),)'
        from_trait_replacement = r'''\1
            ProtobufRequestType::Subscribe => RequestType::Subscribe,
            ProtobufRequestType::PSubscribe => RequestType::PSubscribe,
            ProtobufRequestType::Unsubscribe => RequestType::Unsubscribe,
            ProtobufRequestType::PUnsubscribe => RequestType::PUnsubscribe,
            ProtobufRequestType::SSubscribe => RequestType::SSubscribe,
            ProtobufRequestType::SUnsubscribe => RequestType::SUnsubscribe,
\2'''
        
        if re.search(from_trait_pattern, new_content) and 'ProtobufRequestType::Subscribe =>' not in new_content:
            if not needs_patching:
                create_backup(request_type_file)
            new_content = re.sub(from_trait_pattern, from_trait_replacement, new_content)
            needs_patching = True
            log_message("Applied From trait pub/sub patch")
        elif 'ProtobufRequestType::Subscribe =>' in new_content:
            log_message("From trait already patched")
        
        if needs_patching:
            with open(request_type_file, 'w') as f:
                f.write(new_content)
            log_message(f"Successfully patched RequestType file: {request_type_file}")
            return True
        else:
            log_message(f"RequestType file already patched: {request_type_file}")
            return True
    
    except Exception as e:
        log_message(f"Error patching {request_type_file}: {e}", "ERROR")
        return False

def verify_rust_patch(rust_types_file):
    """Verify that the Rust patch was applied correctly"""
    try:
        with open(rust_types_file, 'r') as f:
            content = f.read()
        
        # Check for the fixed patterns
        jitter_fixed = 'Some(strategy.jitter_percent)' in content
        refresh_fixed = 'refresh_interval_seconds: Some(refresh_interval_seconds)' in content
        
        if jitter_fixed and refresh_fixed:
            log_message("Rust patch verification: SUCCESS")
            return True
        else:
            missing = []
            if not jitter_fixed:
                missing.append("jitter_percent fix")
            if not refresh_fixed:
                missing.append("refresh_interval_seconds fix")
            log_message(f"Rust patch verification: FAILED - Missing: {', '.join(missing)}", "ERROR")
            return False
    
    except Exception as e:
        log_message(f"Error verifying patch: {e}", "ERROR")
        return False

def verify_request_type_patch(request_type_file):
    """Verify that the RequestType patch was applied correctly"""
    try:
        with open(request_type_file, 'r') as f:
            content = f.read()
        
        # Check for the pub/sub patterns
        get_command_fixed = 'RequestType::Subscribe =>' in content
        from_trait_fixed = 'ProtobufRequestType::Subscribe =>' in content
        
        if get_command_fixed and from_trait_fixed:
            log_message("RequestType patch verification: SUCCESS")
            return True
        else:
            missing = []
            if not get_command_fixed:
                missing.append("get_command pub/sub mappings")
            if not from_trait_fixed:
                missing.append("From trait pub/sub mappings")
            log_message(f"RequestType patch verification: FAILED - Missing: {', '.join(missing)}", "ERROR")
            return False
    
    except Exception as e:
        log_message(f"Error verifying RequestType patch: {e}", "ERROR")
        return False

def main():
    """Main function to run all patching operations for protobuf and Rust files"""
    log_message("Starting build-time patching process for protobuf and Rust files")
    
    # Base directory (relative to script location)
    base_dir = Path(__file__).parent.parent
    
    # Define paths
    protobuf_directory = base_dir / "valkey-glide" / "glide-core" / "src" / "protobuf"
    rust_types_file = base_dir / "valkey-glide" / "glide-core" / "src" / "client" / "types.rs"
    request_type_file = base_dir / "valkey-glide" / "glide-core" / "src" / "request_type.rs"
    
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
    
    # Step 3: Patch RequestType pub/sub mappings
    log_message("=== Phase 3: Patching RequestType Pub/Sub Mappings ===")
    if request_type_file.exists():
        if not patch_request_type_rs(str(request_type_file)):
            success = False
        else:
            # Verify the patch
            if not verify_request_type_patch(str(request_type_file)):
                success = False
    else:
        log_message(f"RequestType file not found: {request_type_file}", "ERROR")
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
