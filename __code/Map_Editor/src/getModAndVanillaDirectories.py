import tkinter as tk
from tkinter import filedialog
import os
import re

#Get path of file_directories.txt
default_dir = os.getcwd()
config_path = os.path.join(default_dir, "file_directories.txt")

root = tk.Tk()
root.title("Select Directories")

mod_dir = default_dir
vanilla_dir = default_dir
max_cores = (os.cpu_count() / 3 * 2)

if os.path.exists(config_path):
    with open(config_path, "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r'(\w+)\s*=\s*(.*)', line.strip())
            if not m:
                continue
            key, value = m.groups()
            value = value.replace("\"", "")
            
            # Check if the path is a real directory
            if os.path.isdir(value):
                if key == "mod_directory":
                    mod_dir = value
                elif key == "vanilla_directory":
                    vanilla_dir = value
                    
            elif key == "max_cores" and str.isnumeric(value) and int(value) < os.cpu_count():
                max_cores = int(value)

dir1 = tk.StringVar(value=vanilla_dir)
dir2 = tk.StringVar(value=mod_dir)

def choose_dir(var):
    path = filedialog.askdirectory(initialdir=var.get())
    if path:
        var.set(path)

def submit():
    result = dir1.get() + "\n" + dir2.get() + "\n" + str(coresSlider.get())
    print(result)
    root.destroy()


tk.Label(root, text="Vanilla Directory:").grid(row=0, column=0, padx=10, pady=5, sticky="w")
tk.Entry(root, textvariable=dir1, state="readonly", width=100).grid(row=0, column=1, padx=10, pady=5)
tk.Button(root, text="Browse...", command=lambda: choose_dir(dir1)).grid(row=0, column=2, padx=10, pady=5)

tk.Label(root, text="Mod Directory:").grid(row=1, column=0, padx=10, pady=5, sticky="w")
tk.Entry(root, textvariable=dir2, state="readonly", width=100).grid(row=1, column=1, padx=10, pady=5)
tk.Button(root, text="Browse...", command=lambda: choose_dir(dir2)).grid(row=1, column=2, padx=10, pady=5)

tk.Label(root, text="Max no. of cores:").grid(row=2, column=0, padx=10, pady=5, sticky="w")
coresSlider = tk.Scale(root, from_=1, to=os.cpu_count(), orient=tk.HORIZONTAL, length=180)
coresSlider.grid(row=2, column=1, padx=10, pady=5)
coresSlider.set(max_cores)


tk.Button(root, text="Enter", command=submit).grid(row=3, column=1, pady=15)

root.mainloop()
