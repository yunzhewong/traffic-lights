import threading
import time
import tkinter as tk
from tkinter import ttk

import serial
from crc import read_state, write_times


class RequestedDuration():
    def __init__(self, parent: ttk.LabelFrame, row: int, name: str):
        ttk.Label(parent, text=name, width=12).grid(
            row=row, column=0, padx=10, pady=10, sticky="e"
        )
        self.value = tk.IntVar(value=1)
        self.setpoint = ttk.Spinbox(
            parent, from_=1, to=20, increment=1, textvariable=self.value, width=8
        )
        self.setpoint.grid(row=row, column=1, padx=10, pady=10)
        self.setpoint.bind("<Return>", lambda event: self._confirm_value())

        self.readback = ttk.Label(parent, text=f"{1} s", width=8, style="Readback.TLabel")
        self.readback.grid(row=row, column=2, padx=10, pady=10, sticky="w")

    def _confirm_value(self):
        try:
            value = self.value.get()
            self.readback.config(text=f"{value} s")
        except tk.TclError:
            pass

class ReadbackDuration():
    def __init__(self, parent: ttk.LabelFrame, row: int, name: str):
        ttk.Label(parent, text=name, width=12).grid(
            row=row, column=0, padx=10, pady=10, sticky="e"
        )
        self.value = tk.IntVar(value=1)
        self.readback = ttk.Label(parent, text=f"{1} s", width=8, style="Readback.TLabel")
        self.readback.grid(row=row, column=1, padx=10, pady=10, sticky="w")

class CountdownTimer():
    def __init__(self, parent: ttk.LabelFrame):
        label = ttk.Label(parent, text="5.0/10.0", style="Countdown.TLabel")
        label.grid(
            row=0, column=2,
            columnspan=2, rowspan=2, 
            padx=15, pady=15, 
            sticky="nsew"
        )

class Circle():
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int, r: int, fill: str):
        self.canvas = canvas
        self.ref = canvas.create_oval(
            cx - r, cy -r,    # top-left corner (x1, y1)
            cx + r, cy + r,  # bottom-right corner (x2, y2)
            fill=fill,
            outline="black",
            width=1
        )

    def change_color(self, color: str):
        self.canvas.itemconfig(self.ref, fill=color)

DIM_RED = "#8C3A3A"
DIM_YELLOW = "#A8863A"
DIM_GREEN = "#4A7A57"
DIM_BLUE = "#4A6FA5"

BRIGHT_RED    = "#FF0000"
BRIGHT_YELLOW = "#FFFF00"
BRIGHT_GREEN  = "#00FF00"
BRIGHT_BLUE   = "#0000FF"

class TrafficLight():
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int):
        self.red = Circle(canvas, cx=cx, cy=cy - 40, r=15, fill=DIM_RED)
        self.yellow = Circle(canvas, cx=cx, cy=cy, r=15, fill=DIM_YELLOW)
        self.green = Circle(canvas, cx=cx, cy=cy + 40, r=15, fill=DIM_GREEN)

    def handle_state(self, state: int):
        self.red.change_color(BRIGHT_RED if state & (1 << 2) else DIM_RED)
        self.yellow.change_color(BRIGHT_YELLOW if state & (1 << 1) else DIM_YELLOW)
        self.green.change_color(BRIGHT_GREEN if state & (1 << 0) else DIM_GREEN)


class PedestrianLight():
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int):
        self.red = Circle(canvas, cx=cx, cy=cy - 15, r=10, fill=DIM_RED)
        self.green = Circle(canvas, cx=cx, cy=cy + 15, r=10, fill=DIM_GREEN)

    def handle_state(self, state: int):
        self.red.change_color(BRIGHT_RED if state & (1 << 1) else DIM_RED)
        self.green.change_color(BRIGHT_GREEN if state & (1 << 0) else DIM_GREEN)

class PedestrianPair():
    def __init__(self, first: PedestrianLight, second: PedestrianLight):
        self.first = first
        self.second = second

    def handle_state(self, state: int):
        self.first.handle_state(state)
        self.second.handle_state(state)

class ReqLight():
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int):
        self.circle = Circle(canvas, cx=cx, cy=cy, r=8, fill=DIM_BLUE)

    def handle_state(self, state: int):
        self.circle.change_color(BRIGHT_BLUE if state & (1 << 0) else DIM_BLUE)

class PedRequest():
    def __init__(self, first: ReqLight, second: ReqLight):
        self.first = first
        self.second = second

    def handle_state(self, state: int):
        self.first.handle_state(state)
        self.second.handle_state(state)
        
class Lights():
    def __init__(self, canvas: tk.Canvas):
        self.north = TrafficLight(canvas=canvas, cx=350, cy=100)
        self.south = TrafficLight(canvas=canvas, cx=350, cy=600)
        self.east = TrafficLight(canvas=canvas, cx=100, cy=350)
        self.west = TrafficLight(canvas=canvas, cx=600, cy=350)
        self.north_ped = PedestrianPair(first=PedestrianLight(canvas=canvas, cx=150, cy=100), second=PedestrianLight(canvas=canvas, cx=550, cy=100))
        self.east_ped = PedestrianPair(first=PedestrianLight(canvas=canvas, cx=600, cy=150), second=PedestrianLight(canvas=canvas, cx=600, cy=550))
        self.south_ped = PedestrianPair(first=PedestrianLight(canvas=canvas, cx=150, cy=600), second=PedestrianLight(canvas=canvas, cx=550, cy=600))
        self.west_ped = PedestrianPair(first=PedestrianLight(canvas=canvas, cx=100, cy=150), second=PedestrianLight(canvas=canvas, cx=100, cy=550))
        self.north_req = PedRequest(first=ReqLight(canvas=canvas, cx=175, cy=100), second=ReqLight(canvas=canvas, cx=525, cy=100))
        self.east_req = PedRequest(first=ReqLight(canvas=canvas, cx=600, cy=200), second=ReqLight(canvas=canvas, cx=600, cy=500))
        self.south_req = PedRequest(first=ReqLight(canvas=canvas, cx=175, cy=600), second=ReqLight(canvas=canvas, cx=525, cy=600))
        self.west_req = PedRequest(first=ReqLight(canvas=canvas, cx=100, cy=200), second=ReqLight(canvas=canvas, cx=100, cy=500))

    def handle_state(self, state: bytes):
        print(state.hex())
        self.handle_traffic(state[0])
        self.handle_pedestrian(state[1])
        self.handle_request(state[2])

    def handle_traffic(self, value: int):
        self.north.handle_state(value >> 4)
        self.south.handle_state(value >> 4)
        self.east.handle_state(value)
        self.west.handle_state(value)

    def handle_pedestrian(self, value: int):
        self.north_ped.handle_state(value >> 6)
        self.east_ped.handle_state(value >> 4)
        self.south_ped.handle_state(value >> 2)
        self.west_ped.handle_state(value)

    def handle_request(self, value: int):
        self.north_req.handle_state(value >> 3)
        self.east_req.handle_state(value >> 2)
        self.south_req.handle_state(value >> 1)
        self.west_req.handle_state(value)

if __name__ == "__main__":
    # 1. Create the main window
    root = tk.Tk()
    root.title("My First Tkinter App")
    root.geometry("775x900")  # width x height
    root.columnconfigure(0, weight=1, minsize=300)
    root.columnconfigure(1, weight=1, minsize=200)
    root.columnconfigure(2, weight=1, minsize=150)

    style = ttk.Style()
    style.configure("Readback.TLabel", background="yellow")
    style.configure("Countdown.TLabel", font=("Segoe UI", 24, "bold"), background="yellow")

    # 2. Create some widgets
    requested_duration_group = ttk.LabelFrame(root, text="Requested Durations")
    requested_duration_group.grid(row=0, column=0, padx=15, pady=15, sticky="nsew")
    RequestedDuration(parent=requested_duration_group, row=0, name="North - South")
    RequestedDuration(parent=requested_duration_group, row=1, name="East - West")

    duration_readback_group = ttk.LabelFrame(root, text="Duration Readbacks")
    duration_readback_group.grid(row=0, column=1, padx=15, pady=15, sticky="nsew")
    ReadbackDuration(parent=duration_readback_group, row=0, name="North - South")
    ReadbackDuration(parent=duration_readback_group, row=1, name="East - West")

    transition_countdown_group = ttk.LabelFrame(root, text="Transition")
    transition_countdown_group.grid(row=0, column=2, padx=15, pady=15, sticky="nsew")
    CountdownTimer(transition_countdown_group)

    traffic_group = ttk.LabelFrame(root, text="Traffic State")
    traffic_group.grid(row=1, column=0, columnspan=3, padx=15, sticky="nsew")
    canvas = tk.Canvas(traffic_group, width=700, height=700, bg="white")
    canvas.grid(row=0, column=0, padx=10, pady=10)

    lights = Lights(canvas=canvas)

    event = threading.Event()
    def toggle():
        try:
            device = serial.Serial(port="/dev/ttyACM0", timeout=0.01)
            while not event.is_set():
                lights.handle_state(read_state(device))
                time.sleep(0.01)
        except Exception as e:
            print(e)

    thread = threading.Thread(target=toggle)
    thread.start()


    # Optional: also confirm when pressing Enter in the field
    # 3. Start the event loop (keeps the window open and responsive)
    try:
        root.mainloop()
    except KeyboardInterrupt:
        pass
    finally:
        event.set()
        thread.join()