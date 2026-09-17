import tkinter as tk
from tkinter import ttk


class RequestedDuration():
    def __init__(self, parent: ttk.LabelFrame, row: int, name: str):
        ttk.Label(parent, text=name, width=12).grid(
            row=row, column=0, padx=10, pady=10, sticky="e"
        )
        self.value = tk.IntVar(value=1)
        self.setpoint = ttk.Spinbox(
            group, from_=1, to=20, increment=1, textvariable=self.value, width=8
        )
        self.setpoint.grid(row=row, column=1, padx=10, pady=10)
        self.setpoint.bind("<Return>", lambda event: self._confirm_value())

        self.readback = ttk.Label(group, text=f"{1} s", width=8)
        self.readback.grid(row=row, column=3, padx=10, pady=10, sticky="w")

    def _confirm_value(self):
        try:
            value = self.value.get()
            self.readback.config(text=f"{value} s")
        except tk.TclError:
            pass



if __name__ == "__main__":
    # 1. Create the main window
    root = tk.Tk()
    root.title("My First Tkinter App")
    root.geometry("400x200")  # width x height

    # 2. Create some widgets

    group = ttk.LabelFrame(root, text="Requested Durations")
    group.grid(row=0, column=0, padx=15, pady=15, sticky="nsew")

    RequestedDuration(parent=group, row=0, name="North - South")
    RequestedDuration(parent=group, row=1, name="East - West")




    # Optional: also confirm when pressing Enter in the field
    # 3. Start the event loop (keeps the window open and responsive)
    root.mainloop()