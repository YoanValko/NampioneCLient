#!/usr/bin/env python3
"""
Nemotron Client - External Python GUI
Communicates with injected DLL via named pipes
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import json
import threading
import time
import struct
import sys
import os
from pathlib import Path

# Named pipe communication
PIPE_NAME = r'\\.\pipe\NemotronIPC'

class NemotronGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Nemotron Client - Control Panel")
        self.root.geometry("900x700")
        self.root.minsize(800, 600)
        
        self.connected = False
        self.pipe_handle = None
        self.running = True
        
        self.setup_ui()
        self.start_connection_thread()
        
    def setup_ui(self):
        # Main container
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Header
        header = ttk.Frame(main_frame)
        header.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(header, text="Nemotron Client", font=("Segoe UI", 18, "bold")).pack(side=tk.LEFT)
        ttk.Label(header, text="1.8.9 Forge", font=("Segoe UI", 10), foreground="gray").pack(side=tk.LEFT, padx=(10, 0), pady=(5, 0))
        
        # Connection status
        self.status_frame = ttk.Frame(main_frame)
        self.status_frame.pack(fill=tk.X, pady=(0, 10))
        self.status_indicator = tk.Canvas(self.status_frame, width=12, height=12, highlightthickness=0)
        self.status_indicator.pack(side=tk.LEFT)
        self.status_light = self.status_indicator.create_oval(2, 2, 10, 10, fill="red")
        self.status_label = ttk.Label(self.status_frame, text="Disconnected", font=("Segoe UI", 10))
        self.status_label.pack(side=tk.LEFT, padx=(10, 0))
        ttk.Button(self.status_frame, text="Reconnect", command=self.connect).pack(side=tk.RIGHT)
        
        # Notebook tabs
        notebook = ttk.Notebook(main_frame)
        notebook.pack(fill=tk.BOTH, expand=True)
        
        self.setup_autoclicker_tab(notebook)
        self.setup_esp_tab(notebook)
        self.setup_aimbot_tab(notebook)
        self.setup_logs_tab(notebook)
        
    def setup_autoclicker_tab(self, notebook):
        tab = ttk.Frame(notebook, padding="15")
        notebook.add(tab, text="🖱 AutoClicker")
        
        # Enable toggle
        top_frame = ttk.Frame(tab)
        top_frame.pack(fill=tk.X, pady=(0, 15))
        self.ac_enabled = tk.BooleanVar()
        ttk.Checkbutton(top_frame, text="Enable AutoClicker", variable=self.ac_enabled, 
                       command=lambda: self.send_toggle(1, self.ac_enabled.get())).pack(side=tk.LEFT)
        
        # Settings grid
        settings_frame = ttk.LabelFrame(tab, text="Settings", padding="15")
        settings_frame.pack(fill=tk.BOTH, expand=True)
        
        # CPS Min/Max
        ttk.Label(settings_frame, text="CPS Min:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.ac_cps_min = tk.IntVar(value=8)
        ttk.Spinbox(settings_frame, from_=1, to=20, textvariable=self.ac_cps_min, width=10,
                   command=lambda: self.send_setting(1, "cps_min", self.ac_cps_min.get())).grid(row=0, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        ttk.Label(settings_frame, text="CPS Max:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.ac_cps_max = tk.IntVar(value=12)
        ttk.Spinbox(settings_frame, from_=1, to=20, textvariable=self.ac_cps_max, width=10,
                   command=lambda: self.send_setting(1, "cps_max", self.ac_cps_max.get())).grid(row=1, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        # Options
        self.ac_jitter = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Jitter (±15%)", variable=self.ac_jitter,
                       command=lambda: self.send_setting(1, "jitter", self.ac_jitter.get())).grid(row=2, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        self.ac_left = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Left Click", variable=self.ac_left,
                       command=lambda: self.send_setting(1, "left_click", self.ac_left.get())).grid(row=3, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        self.ac_right = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="Right Click", variable=self.ac_right,
                       command=lambda: self.send_setting(1, "right_click", self.ac_right.get())).grid(row=4, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        self.ac_weapon = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="Only When Holding Weapon", variable=self.ac_weapon,
                       command=lambda: self.send_setting(1, "only_weapon", self.ac_weapon.get())).grid(row=5, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        ttk.Label(settings_frame, text="Hit Delay (ms):").grid(row=6, column=0, sticky=tk.W, pady=5)
        self.ac_hit_delay = tk.IntVar(value=0)
        ttk.Spinbox(settings_frame, from_=0, to=100, textvariable=self.ac_hit_delay, width=10,
                   command=lambda: self.send_setting(1, "hit_delay", self.ac_hit_delay.get())).grid(row=6, column=1, sticky=tk.W, padx=(10, 0), pady=5)
    
    def setup_esp_tab(self, notebook):
        tab = ttk.Frame(notebook, padding="15")
        notebook.add(tab, text="👁 ESP")
        
        # Enable toggle
        top_frame = ttk.Frame(tab)
        top_frame.pack(fill=tk.X, pady=(0, 15))
        self.esp_enabled = tk.BooleanVar()
        ttk.Checkbutton(top_frame, text="Enable ESP", variable=self.esp_enabled,
                       command=lambda: self.send_toggle(2, self.esp_enabled.get())).pack(side=tk.LEFT)
        
        settings_frame = ttk.LabelFrame(tab, text="Settings", padding="15")
        settings_frame.pack(fill=tk.BOTH, expand=True)
        
        self.esp_players = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Players", variable=self.esp_players,
                       command=lambda: self.send_setting(2, "players", self.esp_players.get())).grid(row=0, column=0, sticky=tk.W, pady=5)
        
        self.esp_mobs = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="Mobs", variable=self.esp_mobs,
                       command=lambda: self.send_setting(2, "mobs", self.esp_mobs.get())).grid(row=1, column=0, sticky=tk.W, pady=5)
        
        self.esp_animals = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="Animals", variable=self.esp_animals,
                       command=lambda: self.send_setting(2, "animals", self.esp_animals.get())).grid(row=2, column=0, sticky=tk.W, pady=5)
        
        self.esp_chests = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="Chests", variable=self.esp_chests,
                       command=lambda: self.send_setting(2, "chests", self.esp_chests.get())).grid(row=3, column=0, sticky=tk.W, pady=5)
        
        self.esp_tracers = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Tracers", variable=self.esp_tracers,
                       command=lambda: self.send_setting(2, "tracers", self.esp_tracers.get())).grid(row=4, column=0, sticky=tk.W, pady=5)
        
        self.esp_boxes = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Boxes", variable=self.esp_boxes,
                       command=lambda: self.send_setting(2, "boxes", self.esp_boxes.get())).grid(row=5, column=0, sticky=tk.W, pady=5)
        
        self.esp_health = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Health", variable=self.esp_health,
                       command=lambda: self.send_setting(2, "health", self.esp_health.get())).grid(row=6, column=0, sticky=tk.W, pady=5)
        
        ttk.Label(settings_frame, text="Max Distance:").grid(row=7, column=0, sticky=tk.W, pady=5)
        self.esp_distance = tk.DoubleVar(value=100.0)
        ttk.Spinbox(settings_frame, from_=10, to=500, increment=10, textvariable=self.esp_distance, width=10,
                   command=lambda: self.send_setting(2, "max_distance", self.esp_distance.get())).grid(row=7, column=1, sticky=tk.W, padx=(10, 0), pady=5)
    
    def setup_aimbot_tab(self, notebook):
        tab = ttk.Frame(notebook, padding="15")
        notebook.add(tab, text="🎯 Aimbot")
        
        # Enable toggle
        top_frame = ttk.Frame(tab)
        top_frame.pack(fill=tk.X, pady=(0, 15))
        self.aim_enabled = tk.BooleanVar()
        ttk.Checkbutton(top_frame, text="Enable Aimbot", variable=self.aim_enabled,
                       command=lambda: self.send_toggle(3, self.aim_enabled.get())).pack(side=tk.LEFT)
        
        settings_frame = ttk.LabelFrame(tab, text="Settings", padding="15")
        settings_frame.pack(fill=tk.BOTH, expand=True)
        
        ttk.Label(settings_frame, text="FOV:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.aim_fov = tk.DoubleVar(value=90.0)
        ttk.Spinbox(settings_frame, from_=10, to=180, increment=5, textvariable=self.aim_fov, width=10,
                   command=lambda: self.send_setting(3, "fov", self.aim_fov.get())).grid(row=0, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        ttk.Label(settings_frame, text="Smooth:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.aim_smooth = tk.DoubleVar(value=5.0)
        ttk.Spinbox(settings_frame, from_=1, to=30, increment=0.5, textvariable=self.aim_smooth, width=10,
                   command=lambda: self.send_setting(3, "smooth", self.aim_smooth.get())).grid(row=1, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        self.aim_prediction = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Movement Prediction", variable=self.aim_prediction,
                       command=lambda: self.send_setting(3, "prediction", self.aim_prediction.get())).grid(row=2, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        self.aim_autoswitch = tk.BooleanVar(value=False)
        ttk.Checkbutton(settings_frame, text="Auto Switch Target", variable=self.aim_autoswitch,
                       command=lambda: self.send_setting(3, "auto_switch", self.aim_autoswitch.get())).grid(row=3, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        ttk.Label(settings_frame, text="Target Mode:").grid(row=4, column=0, sticky=tk.W, pady=5)
        self.aim_target_mode = tk.IntVar(value=0)
        ttk.Combobox(settings_frame, textvariable=self.aim_target_mode, values=[0, 1, 2], width=8,
                    state="readonly").grid(row=4, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        ttk.Label(settings_frame, text="(0=Closest, 1=Lowest HP, 2=Looking At)").grid(row=4, column=2, sticky=tk.W, padx=(10, 0))
        
        ttk.Label(settings_frame, text="Hitbox:").grid(row=5, column=0, sticky=tk.W, pady=5)
        self.aim_hitbox = tk.IntVar(value=0)
        ttk.Combobox(settings_frame, textvariable=self.aim_hitbox, values=[0, 1, 2], width=8,
                    state="readonly").grid(row=5, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        ttk.Label(settings_frame, text="(0=Head, 1=Body, 2=Random)").grid(row=5, column=2, sticky=tk.W, padx=(10, 0))
        
        ttk.Label(settings_frame, text="Max Distance:").grid(row=6, column=0, sticky=tk.W, pady=5)
        self.aim_distance = tk.DoubleVar(value=50.0)
        ttk.Spinbox(settings_frame, from_=10, to=200, increment=10, textvariable=self.aim_distance, width=10,
                   command=lambda: self.send_setting(3, "max_distance", self.aim_distance.get())).grid(row=6, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        self.aim_visible = tk.BooleanVar(value=True)
        ttk.Checkbutton(settings_frame, text="Visible Check", variable=self.aim_visible,
                       command=lambda: self.send_setting(3, "visible_check", self.aim_visible.get())).grid(row=7, column=0, columnspan=2, sticky=tk.W, pady=5)
        
        # Humanization settings
        human_frame = ttk.LabelFrame(tab, text="Humanization (Anti-Cheat Bypass)", padding="15")
        human_frame.pack(fill=tk.X, pady=(15, 0))
        
        ttk.Label(human_frame, text="Reaction Time (ms):").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.aim_reaction = tk.IntVar(value=100)
        ttk.Spinbox(human_frame, from_=0, to=500, increment=10, textvariable=self.aim_reaction, width=10).grid(row=0, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        ttk.Label(human_frame, text="Overshoot (%):").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.aim_overshoot = tk.DoubleVar(value=5.0)
        ttk.Spinbox(human_frame, from_=0, to=20, increment=1, textvariable=self.aim_overshoot, width=10).grid(row=1, column=1, sticky=tk.W, padx=(10, 0), pady=5)
        
        ttk.Label(human_frame, text="Jitter:").grid(row=2, column=0, sticky=tk.W, pady=5)
        self.aim_jitter = tk.DoubleVar(value=0.5)
        ttk.Spinbox(human_frame, from_=0, to=5, increment=0.1, textvariable=self.aim_jitter, width=10).grid(row=2, column=1, sticky=tk.W, padx=(10, 0), pady=5)
    
    def setup_logs_tab(self, notebook):
        tab = ttk.Frame(notebook, padding="10")
        notebook.add(tab, text="📋 Logs")
        
        self.log_text = scrolledtext.ScrolledText(tab, font=("Consolas", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True)
        self.log_text.config(state=tk.DISABLED)
        
        btn_frame = ttk.Frame(tab)
        btn_frame.pack(fill=tk.X, pady=(10, 0))
        ttk.Button(btn_frame, text="Clear", command=self.clear_logs).pack(side=tk.LEFT)
        ttk.Button(btn_frame, text="Save", command=self.save_logs).pack(side=tk.LEFT, padx=(5, 0))
    
    def log(self, msg, level="INFO"):
        import datetime
        ts = datetime.datetime.now().strftime("%H:%M:%S")
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, f"[{ts}] {level}: {msg}\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def clear_logs(self):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def save_logs(self):
        from tkinter import filedialog
        fname = filedialog.asksaveasfilename(defaultextension=".log", filetypes=[("Log files", "*.log")])
        if fname:
            with open(fname, 'w') as f:
                f.write(self.log_text.get(1.0, tk.END))
            messagebox.showinfo("Saved", f"Logs saved to {fname}")
    
    # Pipe communication
    def connect(self):
        self.log("Connecting to injected DLL...")
        threading.Thread(target=self._connect_thread, daemon=True).start()
    
    def _connect_thread(self):
        import win32file
        import win32pipe
        import pywintypes
        
        while self.running:
            try:
                self.pipe_handle = win32file.CreateFile(
                    PIPE_NAME,
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0, None,
                    win32file.OPEN_EXISTING,
                    0, None
                )
                
                self.root.after(0, lambda: self.on_connected())
                self.log("Connected to Nemotron DLL!")
                
                # Read loop
                while self.running:
                    try:
                        result, data = win32file.ReadFile(self.pipe_handle, 4096)
                        if data:
                            self.handle_response(data)
                    except pywintypes.error as e:
                        if e.winerror in (109, 232):  # Broken pipe
                            break
                        raise
                        
            except pywintypes.error as e:
                if e.winerror == 2:  # File not found
                    self.root.after(0, lambda: self.on_disconnected())
                    time.sleep(1)
                else:
                    self.log(f"Pipe error: {e}", "ERROR")
                    time.sleep(1)
            except Exception as e:
                self.log(f"Connection error: {e}", "ERROR")
                time.sleep(1)
    
    def on_connected(self):
        self.connected = True
        self.status_indicator.itemconfig(self.status_light, fill="green")
        self.status_label.config(text="Connected")
        self.request_status()
    
    def on_disconnected(self):
        self.connected = False
        self.status_indicator.itemconfig(self.status_light, fill="red")
        self.status_label.config(text="Disconnected")
    
    def send_command(self, command, module_type, data):
        if not self.connected or not self.pipe_handle:
            return False
        
        import win32file
        import json
        
        payload = {
            "command": command,
            "moduleType": module_type,
            "data": json.dumps(data)
        }
        
        try:
            data = json.dumps(payload).encode('utf-8')
            # Pack: 4-byte length + payload
            packed = struct.pack('<I', len(data)) + data
            win32file.WriteFile(self.pipe_handle, packed)
            return True
        except Exception as e:
            self.log(f"Send error: {e}", "ERROR")
            return False
    
    def send_toggle(self, module_type, enable):
        self.send_command(1, module_type, {"enable": enable})
    
    def send_setting(self, module_type, key, value):
        data = {key: value}
        self.send_command(2, module_type, data)
    
    def request_status(self):
        self.send_command(3, 0, {})
    
    def handle_response(self, data):
        import json
        try:
            response = json.loads(data.decode('utf-8'))
            self.log(f"Response: {response}")
        except:
            pass

def main():
    # Check for required packages
    try:
        import win32file
        import win32pipe
        import pywintypes
    except ImportError:
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "pywin32"])
        import win32file
        import win32pipe
        import pywintypes
    
    root = tk.Tk()
    app = NemotronGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()