from abc import ABC, abstractmethod
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk
from typing import Callable, Generic, Optional, TypeVar, final

from ai import Response, into_response
from crc import Times, USBCommunications


class RequestedDuration():
    def __init__(self, parent: ttk.LabelFrame, row: int, name: str, handle_change: Callable[[], None]):
        ttk.Label(parent, text=name, width=12).grid(
            row=row, column=0, padx=10, pady=10, sticky="e"
        )
        self.last_value = 1
        self.value = tk.IntVar(value=self.last_value)
        self.setpoint = ttk.Spinbox(
            parent, from_=1, to=20, increment=1, textvariable=self.value, width=8
        )
        self.setpoint.grid(row=row, column=1, padx=10, pady=10)
        self.setpoint.bind("<Return>", lambda event: self._confirm_value())

        self.readback = ttk.Label(parent, text=f"{1} s", width=8, style="Readback.TLabel")
        self.readback.grid(row=row, column=2, padx=10, pady=10, sticky="w")
        self.handle_change = handle_change

    def _confirm_value(self):
        try:
            value = int(self.value.get())
            self.readback.config(text=f"{value} s")
            self.last_value = value
            self.handle_change()
        except tk.TclError:
            pass

    def get_value(self):
        return self.last_value

class DurationRequests():
    def __init__(self, parent: ttk.LabelFrame, handle_times: Callable[[Times], None]):
        self.north_south = RequestedDuration(parent=parent, row=0, name="North - South", handle_change=self.handle_change)
        self.east_west = RequestedDuration(parent=parent, row=1, name="East - West", handle_change=self.handle_change)
        self.handle_times = handle_times

    def handle_change(self):
        self.handle_times(Times(north_south=self.north_south.get_value(), east_west=self.east_west.get_value()))

class ReadbackDuration():
    def __init__(self, parent: ttk.LabelFrame, row: int, name: str):
        ttk.Label(parent, text=name, width=12).grid(
            row=row, column=0, padx=10, pady=10, sticky="e"
        )
        self.value = tk.IntVar(value=1)
        self.readback = ttk.Label(parent, text=f"{1} s", width=8, style="Readback.TLabel")
        self.readback.grid(row=row, column=1, padx=10, pady=10, sticky="w")

    def change_value(self, value: int):
        self.readback.config(text=f"{value} s")

class CountdownTimer():
    def __init__(self, parent: ttk.LabelFrame):
        self.label = ttk.Label(parent, text="0.0/5.0", style="Countdown.TLabel")
        self.label.grid(
            row=0, column=2,
            columnspan=2, rowspan=2, 
            padx=15, pady=15, 
            sticky="nsew"
        )

    def change_value(self, time_since_transition: float, transition_time: float):
        self.label.config(text=f"{time_since_transition:.1f}/{transition_time:.1f}")

class Status():
    def __init__(self, parent: tk.Tk, row: int):
        status_group = ttk.LabelFrame(parent, text="Status")
        status_group.grid(row=row, column=0, columnspan=3, padx=15, sticky="nsew")
        ttk.Label(status_group, text="Communications", width=12).grid(
            row=row, column=0, padx=10, pady=10, sticky="e"
        )
        self.label = ttk.Label(status_group, text="Disconnected", style="Readback.TLabel")
        self.label.grid(
            row=row, column=2,
            padx=10, pady=10, 
            sticky="nsew"
        )

    def _set_text(self, t: str):
        self.label.config(text=t)

    def set_connected(self):
        self._set_text("Connected")

    def set_disconnected(self, err_str: str):
        self._set_text(f"Disconnected: {err_str}")


class Header():
    def __init__(self, parent: tk.Tk, row: int, request_queue: queue.Queue[Times]):
        requested_duration_group = ttk.LabelFrame(parent, text="Manually Requested Durations")
        requested_duration_group.grid(row=row, column=0, padx=15, sticky="nsew")
    
        self.duration_requests = DurationRequests(parent=requested_duration_group, handle_times=request_queue.put)
    
        duration_readback_group = ttk.LabelFrame(parent, text="Duration Readbacks")
        duration_readback_group.grid(row=row, column=1, padx=15, sticky="nsew")
        self.north_south_readback = ReadbackDuration(parent=duration_readback_group, row=0, name="North - South")
        self.east_west_readback = ReadbackDuration(parent=duration_readback_group, row=1, name="East - West")
    
        transition_countdown_group = ttk.LabelFrame(parent, text="Transition")
        transition_countdown_group.grid(row=row, column=2, padx=15, sticky="nsew")
        self.countdown_timer = CountdownTimer(transition_countdown_group)

    def change_readbacks(self, times: Times):
        self.north_south_readback.change_value(times.north_south)
        self.east_west_readback.change_value(times.east_west)

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
BRIGHT_BLUE   = "#00FFFF"

class TrafficLight():
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int):
        self.red = Circle(canvas, cx=cx, cy=cy - 40, r=15, fill=DIM_RED)
        self.yellow = Circle(canvas, cx=cx, cy=cy, r=15, fill=DIM_YELLOW)
        self.green = Circle(canvas, cx=cx, cy=cy + 40, r=15, fill=DIM_GREEN)

    def handle_state(self, state: int):
        self.red.change_color(BRIGHT_RED if state & (1 << 2) else DIM_RED)
        self.yellow.change_color(BRIGHT_YELLOW if state & (1 << 1) else DIM_YELLOW)
        self.green.change_color(BRIGHT_GREEN if state & (1 << 0) else DIM_GREEN)

class Light(ABC):
    @abstractmethod
    def handle_state(self, state: int):
        pass

@final
class PedestrianLight(Light):
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int):
        self.red = Circle(canvas, cx=cx, cy=cy - 15, r=10, fill=DIM_RED)
        self.green = Circle(canvas, cx=cx, cy=cy + 15, r=10, fill=DIM_GREEN)

    def handle_state(self, state: int):
        self.red.change_color(BRIGHT_RED if state & (1 << 1) else DIM_RED)
        self.green.change_color(BRIGHT_GREEN if state & (1 << 0) else DIM_GREEN)

@final
class ReqLight(Light):
    def __init__(self, canvas: tk.Canvas, cx: int, cy: int):
        self.circle = Circle(canvas, cx=cx, cy=cy, r=8, fill=DIM_BLUE)

    def handle_state(self, state: int):
        self.circle.change_color(BRIGHT_BLUE if state & (1 << 0) else DIM_BLUE)

T = TypeVar("T", bound=Light)
class LightPair(Generic[T]):
    def __init__(self, first: T, second: T):
        self.first = first
        self.second = second

    def handle_state(self, state: int):
        self.first.handle_state(state)
        self.second.handle_state(state)

def create_text(canvas: tk.Canvas, cx: int, cy: int, text: str):
    return canvas.create_text(
        cx, cy,           # x, y coordinates (center of text by default)
        text=text,
        font=("Arial", 16),
        fill="black",
        anchor="center"      # options: n, s, e, w, center, nw, ne, sw, se
    )


class Lights():
    def __init__(self, canvas: tk.Canvas):
        self.north = TrafficLight(canvas=canvas, cx=350, cy=100)
        create_text(canvas, 350, 25, "North")
        self.south = TrafficLight(canvas=canvas, cx=350, cy=600)
        create_text(canvas, 350, 675, "South")
        self.east = TrafficLight(canvas=canvas, cx=100, cy=350)
        create_text(canvas, 50, 350, "East")
        self.west = TrafficLight(canvas=canvas, cx=600, cy=350)
        create_text(canvas, 650, 350, "West")
        self.north_ped = LightPair(first=PedestrianLight(canvas=canvas, cx=150, cy=100), second=PedestrianLight(canvas=canvas, cx=550, cy=100))
        self.east_ped = LightPair(first=PedestrianLight(canvas=canvas, cx=600, cy=150), second=PedestrianLight(canvas=canvas, cx=600, cy=550))
        self.south_ped = LightPair(first=PedestrianLight(canvas=canvas, cx=150, cy=600), second=PedestrianLight(canvas=canvas, cx=550, cy=600))
        self.west_ped = LightPair(first=PedestrianLight(canvas=canvas, cx=100, cy=150), second=PedestrianLight(canvas=canvas, cx=100, cy=550))
        self.north_req = LightPair(first=ReqLight(canvas=canvas, cx=175, cy=100), second=ReqLight(canvas=canvas, cx=525, cy=100))
        self.east_req = LightPair(first=ReqLight(canvas=canvas, cx=600, cy=200), second=ReqLight(canvas=canvas, cx=600, cy=500))
        self.south_req = LightPair(first=ReqLight(canvas=canvas, cx=175, cy=600), second=ReqLight(canvas=canvas, cx=525, cy=600))
        self.west_req = LightPair(first=ReqLight(canvas=canvas, cx=100, cy=200), second=ReqLight(canvas=canvas, cx=100, cy=500))

    def handle_state(self, state: bytes):
        self._handle_traffic(state[0])
        self._handle_pedestrian(state[1])
        self._handle_request(state[2])
        time_since_transition = state[3] / 10
        transition_cutoff = state[4] / 10
        return time_since_transition, transition_cutoff

    def _handle_traffic(self, value: int):
        self.north.handle_state(value >> 4)
        self.south.handle_state(value >> 4)
        self.east.handle_state(value)
        self.west.handle_state(value)

    def _handle_pedestrian(self, value: int):
        self.north_ped.handle_state(value >> 6)
        self.east_ped.handle_state(value >> 4)
        self.south_ped.handle_state(value >> 2)
        self.west_ped.handle_state(value)

    def _handle_request(self, value: int):
        self.north_req.handle_state(value >> 3)
        self.east_req.handle_state(value >> 2)
        self.south_req.handle_state(value >> 1)
        self.west_req.handle_state(value)

class TrafficCanvas():
    def __init__(self, parent: tk.Tk, row: int):
        traffic_group = ttk.LabelFrame(parent, text="Traffic State")
        traffic_group.grid(row=row, column=0, columnspan=3, padx=15, sticky="nsew")
        canvas = tk.Canvas(traffic_group, width=700, height=700, bg="white")
        canvas.grid(row=0, column=0, padx=10, pady=10)
        self.lights = Lights(canvas=canvas)

    def handle_state(self, state: bytes):
        return self.lights.handle_state(state)

class AIInput():
    def __init__(self, parent: tk.Tk, on_run: Callable[[str, str], None]):
        ai_input = ttk.LabelFrame(parent, text="AI Input")
        ai_input.grid(row=0, column=3, rowspan=2, sticky="nsew")

        ns_label = tk.Label(ai_input, text="North/South:")
        ns_label.grid(row=0, column=0, padx=5, pady=10)
        self.ns_entry = tk.Entry(ai_input, width=15)
        self.ns_entry.grid(row=0, column=1, pady=10)

        ew_label = tk.Label(ai_input, text="East/West:")
        ew_label.grid(row=1, column=0, padx=5, pady=10)
        self.ew_entry = tk.Entry(ai_input, width=15)
        self.ew_entry.grid(row=1, column=1, pady=10)

        button = tk.Button(ai_input, text="Submit", command=self.handle_submit)
        button.grid(row=2, column=0, columnspan=2, pady=10)

        self.on_run = on_run 

    def handle_submit(self):
        self.on_run(self.ns_entry.get(), self.ew_entry.get())

class AIResponse():
    def __init__(self, parent: tk.Tk):
        ai_response = ttk.LabelFrame(parent, text="AI Response")
        ai_response.grid(row=2, column=3, sticky="nsew")
        self.state_label = ttk.Label(ai_response, text=f"Done", width=25, style="Readback.TLabel")
        self.state_label.grid(row=0, column=0, padx=10, pady=10, sticky="w")
        self.label = ttk.Label(ai_response, text=f"", width=25, wraplength=200, style="Readback.TLabel")
        self.label.grid(row=1, column=0, padx=10, pady=10, sticky="w")

    def change_text(self, text: str):
        self.label.config(text=text)

    def set_finished(self, is_finished: bool):
        if is_finished:
            self.state_label.config(text="Done")
        else:
            self.state_label.config(text="Generating...")

class AI():
    def __init__(self, parent: tk.Tk, request_queue: queue.Queue[Times]):
        self.input = AIInput(parent, on_run=self.on_run)
        self.response_ui = AIResponse(parent=parent)

        self.request_queue = request_queue
        self.request_sent = False
        self.last_response: Optional[Response] = None

    def on_change(self):
        if self.last_response is None:
            return
        if self.last_response.done_event.is_set():
            self.response_ui.set_finished(True)

        if not self.request_sent:
            parsed_times = self.last_response.parse()
            if parsed_times is not None:
                self.request_queue.put(parsed_times)
                self.request_sent = False

        try:
            self.response_ui.change_text(self.last_response.get_details())
        except:
            pass

    def on_run(self, north_south: str, east_west: str):
        if self.last_response is not None:
            if not self.last_response.done_event.is_set():
                return

        self.request_sent = False
        self.response_ui.set_finished(False)
        self.last_response = Response(on_change=self.on_change)
        thread = threading.Thread(target=into_response, args=(self.last_response, north_south, east_west))
        thread.start()

if __name__ == "__main__":
    # 1. Create the main window
    root = tk.Tk()
    root.title("Traffic Light Control GUI")
    root.geometry("1010x950")  # width x height
    root.columnconfigure(0, weight=1, minsize=300)
    root.columnconfigure(1, weight=1, minsize=200)
    root.columnconfigure(2, weight=1, minsize=150)
    root.columnconfigure(3, weight=1, minsize=250)

    request_queue = queue.Queue[Times]()

    style = ttk.Style()
    style.configure("Readback.TLabel", background="yellow")
    style.configure("Countdown.TLabel", font=("Segoe UI", 24, "bold"), background="yellow")
    status = Status(parent=root, row=0)
    header = Header(parent=root, row=1, request_queue=request_queue)
    traffic_canvas = TrafficCanvas(parent=root, row=2)

    ai = AI(parent=root, request_queue=request_queue)

    event = threading.Event()
    def toggle():
        while not event.is_set():
            try:
                comms = USBCommunications(port="/dev/ttyACM0")
                initial_readback = comms.read_times()
                header.change_readbacks(initial_readback)
                status.set_connected()

                while not event.is_set():
                    if request_queue.empty():
                        time_since_transition, transition_time = traffic_canvas.handle_state(comms.read_state())
                        header.countdown_timer.change_value(time_since_transition=time_since_transition, transition_time=transition_time)
                    else:
                        change_request = request_queue.get()
                        readback = comms.write_times(change_request)
                        header.change_readbacks(readback)
                    time.sleep(0.01)
            except Exception as e:
                status.set_disconnected(str(e))
            time.sleep(0.5)

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