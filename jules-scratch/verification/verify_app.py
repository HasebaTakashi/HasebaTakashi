import subprocess
import time
import os
import signal
from playwright.sync_api import sync_playwright, expect

def run_verification():
    processes = []
    log_files = {}

    services = {
        "broker": {"cmd": "npm start", "cwd": "./broker"},
        "backend": {"cmd": "npm start", "cwd": "./backend"},
        "webapp": {"cmd": "npm start", "cwd": "./webapp"}
    }

    try:
        # Start all services
        for name, config in services.items():
            log_path = f"jules-scratch/verification/{name}.log"
            log_files[name] = open(log_path, "w")
            print(f"Starting {name} in {config['cwd']}...")
            proc = subprocess.Popen(
                config['cmd'],
                shell=True,
                cwd=config['cwd'], # Set the working directory
                stdout=log_files[name],
                stderr=subprocess.STDOUT,
                preexec_fn=os.setpgrp
            )
            processes.append(proc)
            print(f"  -> PID: {proc.pid}, Log: {log_path}")

        # Wait for services to be ready
        print("Waiting for services to initialize...")
        time.sleep(10) # Wait a fixed time

        # Check webapp log for confirmation
        with open(log_files["webapp"].name, "r") as f:
            logs = f.read()
            if "Web server is listening on port 3000" not in logs:
                print("Web server failed to start. Dumping all logs.")
                for name, f_handle in log_files.items():
                    f_handle.close() # Close before reading
                    with open(f_handle.name, "r") as f_read:
                        print(f"\n--- LOGS FOR {name} ---")
                        print(f_read.read())
                raise RuntimeError("Web server did not start correctly.")

        with sync_playwright() as p:
            browser = p.chromium.launch(headless=True)
            page = browser.new_page()

            print("Navigating to http://localhost:3000")
            page.goto("http://localhost:3000")

            print("Verifying initial content...")
            expect(page.get_by_role("heading", name="Real-time Data Viewer")).to_be_visible()
            expect(page.locator("#thresholds-list li:nth-child(1) > span")).not_to_have_text("N/A", timeout=10000)
            print("Initial content verified.")

            screenshot_path = "jules-scratch/verification/verification.png"
            page.screenshot(path=screenshot_path)
            print(f"Screenshot saved to {screenshot_path}")

            browser.close()

    finally:
        # Clean up processes and log files
        print("Stopping all services...")
        for proc in processes:
            try:
                # Check if process exists before trying to kill it
                if proc.poll() is None:
                    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
            except ProcessLookupError:
                pass # Process already terminated

        for f in log_files.values():
            f.close()

        print("Services stopped.")

if __name__ == "__main__":
    try:
        run_verification()
        print("\n✅ Verification successful!")
    except Exception as e:
        print(f"\n❌ Verification failed: {e}")
        exit(1)
