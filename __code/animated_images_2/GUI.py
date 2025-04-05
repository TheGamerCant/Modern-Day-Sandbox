from PIL import Image
import os
import tkinter as tk
import subprocess

class ToolTip:
    def __init__(self, widget, text):
        self.widget = widget
        self.text = text
        self.tooltip_window = None

        self.widget.bind("<Enter>", self.show_tooltip)
        self.widget.bind("<Leave>", self.hide_tooltip)

    def show_tooltip(self, event=None):
        """Create a tooltip window near the widget."""
        x = self.widget.winfo_rootx() + 20
        y = self.widget.winfo_rooty() + 20

        self.tooltip_window = tk.Toplevel(self.widget)
        self.tooltip_window.wm_overrideredirect(True)  # Remove window border
        self.tooltip_window.geometry(f"+{x}+{y}")

        label = tk.Label(self.tooltip_window, text=self.text, background="yellow", relief="solid", borderwidth=1, font=("Arial", 10))
        label.pack()

    def hide_tooltip(self, event=None):
        """Destroy the tooltip window."""
        if self.tooltip_window:
            self.tooltip_window.destroy()
            self.tooltip_window = None

def main():
    def run_cpp_program():
        var01 = str(width)
        var02 = str(height)
        var03 = xWidth.get()
        var04 = yHeight.get()
        var05 = FPS.get()
        var06 = OutputFilePrefix.get()
        var07 = OutputFileDirectory.get()
        var08 = StartingXPos.get()
        var09 = StartingYPos.get()
        var10 = str(loopingVar.get())
        var11 = str(alwaysTransparentVar.get())
        var12 = str(transparencyCheckVar.get())
        var13 = str(coresSlider.get())
        
        try:
            result = subprocess.run(["./animatedImages", var01, var02, var03, var04, var05, var06, var07, var08, var09, var10, var11, var12, var13], capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
            print(result.stdout)
        except Exception as e:
            print(f"Error: {e}")
        
    # Get video data
    directories = os.listdir('in')
    directories = ["in/" + entry for entry in directories if entry.endswith(".png")]

    img = Image.open(directories[0])
    width, height = img.size
    img.close()


    # GUI
    root = tk.Tk()
    root.title("GUI")

    xWidthLabel = tk.Label(root, text="Enter the width of your output images:")
    xWidthLabel.pack()
    ToolTip(xWidthLabel, "Enter an integer value if you want all images to be a set width, except for\nthe last if the image width is not divisible by the number you've entered.\n\nOtherwise the images will be of roughly equal width with no more than\na difference of 1, for example [12,12,12,11,11,11, etc.].\nRecommended to put a number here if you are using a gridBox.")
    xWidth = tk.Entry(root, width = 28)
    xWidth.pack()
    xWidth.insert(0, "-1")

    yHeightLabel = tk.Label(root, text="Enter the height of your output images:")
    yHeightLabel.pack()
    ToolTip(yHeightLabel, "Determines the height of the image, defaults to the height of your input images.")
    yHeight = tk.Entry(root, width = 28)
    yHeight.pack()
    yHeight.insert(0, f"{height}")

    FPSLabel = tk.Label(root, text="Enter the FPS of your video:")
    FPSLabel.pack()
    ToolTip(FPSLabel, "Used for the output .gfx file.")
    FPS = tk.Entry(root, width = 28)
    FPS.pack()
    FPS.insert(0, "30.0")

    OutputFilePrefixLabel = tk.Label(root, text="Enter the output file prefix:")
    OutputFilePrefixLabel.pack()
    ToolTip(OutputFilePrefixLabel, "Files will be \"[this]_[no.].png\".")
    OutputFilePrefix = tk.Entry(root, width = 28)
    OutputFilePrefix.pack()
    OutputFilePrefix.insert(0, "my_hoi_animation")

    OutputFileDirectoryLabel = tk.Label(root, text="Enter the output file directory:")
    OutputFileDirectoryLabel.pack()
    ToolTip(OutputFileDirectoryLabel, "The directory of your files from the base hoi4 folder.")
    OutputFileDirectory = tk.Entry(root, width = 28)
    OutputFileDirectory.pack()
    OutputFileDirectory.insert(0, "gfx/interface/animated")

    StartingXPosLabel = tk.Label(root, text="Enter the starting X position of your animation:")
    StartingXPosLabel.pack()
    ToolTip(StartingXPosLabel, "Used in the interface/.gui file.")
    StartingXPos = tk.Entry(root, width = 28)
    StartingXPos.pack()
    StartingXPos.insert(0, "0")

    StartingYPosLabel = tk.Label(root, text="Enter the starting Y position of your animation:")
    StartingYPosLabel.pack()
    ToolTip(StartingYPosLabel, "Used in the interface/.gui file.")
    StartingYPos = tk.Entry(root, width = 28)
    StartingYPos.pack()
    StartingYPos.insert(0, "0")

    loopingVar = tk.IntVar(value = 0) 
    loopingCheckBox = tk.Checkbutton(root, text = "looping", variable = loopingVar, onvalue = 1, offvalue = 0, height = 2, width = 20) 
    loopingCheckBox.pack()
    ToolTip(loopingCheckBox, "Does your animation play once or go on forever?")

    alwaysTransparentVar = tk.IntVar(value = 0) 
    alwaysTransparentCheckBox = tk.Checkbutton(root, text = "alwaysTransparent", variable = alwaysTransparentVar, onvalue = 1, offvalue = 0, height = 2, width = 20) 
    alwaysTransparentCheckBox.pack()
    ToolTip(alwaysTransparentCheckBox, "Should the user be able to click through your animation?")

    transparencyCheckVar = tk.IntVar(value = 0) 
    transparencyCheckCheckBox = tk.Checkbutton(root, text = "transparencyCheck", variable = transparencyCheckVar, onvalue = 1, offvalue = 0, height = 2, width = 20) 
    transparencyCheckCheckBox.pack()
    ToolTip(transparencyCheckCheckBox, "User can click through the animation if the alpha channel of a pixel is 0.")

    coresSlider = tk.Scale(root, from_=1, to=os.cpu_count(), orient=tk.HORIZONTAL, length=180)
    coresSlider.pack()
    coresSlider.set(int(os.cpu_count() / 3 * 2))
    ToolTip(coresSlider, "How many cores are we to use for multiprocessing?\n\nMore cores means the program will finish faster, however too\nmany will cause your computer to lag significantly.")

    
    run_button = tk.Button(root, text="Run Program", command=run_cpp_program)
    run_button.pack()

    root.mainloop()

if __name__ == "__main__":
    main()
