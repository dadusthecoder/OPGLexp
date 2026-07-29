import subprocess
import sys
import os

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)
    exe_path = os.path.join(root_dir, 'build', 'bin', 'Debug', 'OPGLexp.exe')
    
    if not os.path.exists(exe_path):
        print(f"Error: Executable not found at {exe_path}. Did you build the project?")
        return

    # Path to LLVM LLDB on Windows
    lldb_path = r'C:\Program Files\LLVM\bin\lldb.exe'
    if not os.path.exists(lldb_path):
        lldb_path = 'lldb' # Fallback to PATH

    print(f"Checking if LLDB is available at {lldb_path}...")
    lldb_works = False
    try:
        check = subprocess.run([lldb_path, '--version'], capture_output=True)
        if check.returncode == 0:
            lldb_works = True
    except Exception:
        pass

    if lldb_works:
        print(f"Launching LLDB for {exe_path}...")
        cmd = [
            lldb_path,
            '--batch',
            '-o', 'run',
            '-o', 'bt',
            '-o', 'quit',
            exe_path
        ]
    else:
        print(f"LLDB not functional or missing DLLs. Falling back to direct execution of {exe_path}...")
        cmd = [exe_path]

    try:
        # Capture both stdout and stderr (sanitizer outputs often go to stderr)
        result = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace', cwd=root_dir)
        
        output = result.stdout + "\n" + result.stderr
        print(output)
        
        # Save output to a log file for review
        log_file = os.path.join(root_dir, 'debug_crash.log')
        with open(log_file, 'w', encoding='utf-8') as f:
            f.write(output)
            
        print(f"\nExecution finished with exit code {result.returncode}. Log saved to {log_file}.")
        
    except Exception as e:
        print(f"Error running program: {e}")

if __name__ == '__main__':
    main()
