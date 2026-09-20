# To run this code you need to install the following dependencies:
# pip install google-genai

import threading
import time
from typing import Callable, Optional

from google import genai
from dotenv import load_dotenv

from crc import Times

load_dotenv()

def generate(input: str, on_streamed_text: Callable[[str], None], on_done: Callable[[], None]):
    client = genai.Client()
    stream = client.interactions.create(
        model="gemini-3.1-flash-lite",
        input=input,
        stream=True
    )
    for event in stream:
        if event.event_type == "step.delta":
            if event.delta.type == "text":
                on_streamed_text(event.delta.text)
        if event.event_type == "interaction.completed":
            on_done()


def format_input(north_south_text: str, east_west_text: str):
    return f""" 
    I have a traffic light that I need help figuring out my traffic green durations for. Can you help me figure out ideal traffic conditions?

    North/South: {north_south_text}
    East/West: {east_west_text}
    
    Return two space separated integers (north/south green duration, east/west green duration) indicating the duration in seconds (in range >=1 and <=20)
    in the first line in the response, and then explain reasoning in the following lines. 
    
    Where possible, do not use the exact numerical ratio to figure out how long each duration should be. Be creative!  
    """ 

class Response():
    def __init__(self, on_change: Callable[[], None]):
        self._lock = threading.Lock()
        self._text = ""
        self.done_event = threading.Event()
        self.on_change = on_change

    def append(self, new_text: str):
        with self._lock:
            self._text += new_text
        self.on_change()

    def parse(self) -> Optional[Times]:
        try:
            lines = self._text.split("\n")
            if len(lines) <= 1:
                return None
            first_line = lines[0]
            numbers = first_line.split(" ")
            if len(numbers) != 2:
                return None
            ints = [int(n) for n in numbers]
            return Times(north_south=ints[0], east_west=ints[1])
        except:
            return None

    def get_details(self):
        try:
            lines = self._text.split("\n")
            if len(lines) <= 1:
                return ""
            return "\n".join(lines[1:])
        except:
            return ""

def sim_ai(on_streamed_text: Callable[[str], None], on_done: Callable[[], None]):
    events = [
        "5 15\n",
        "Since rhinos move with the deliberate, heavy grace",
        "of ancient stone, they require very little time to cross—a short window of green is all they need to lumber across the intersection. Cats",
        ", however, are notoriously indecisive. They will likely pause in the middle of the road to chase a butterfly,",
        " groom a paw, or contemplate the existential dread of a nearby sidewalk crack. Because the feline traffic is dense and prone to sudden",
        " naps, they require significantly more time to clear the intersection."
    ]
    
    for event in events:
        on_streamed_text(event)
        time.sleep(0.1)

    on_done()

def into_response(response: Response, north_south: str, east_west: str): 
    def on_streamed_text(s: str):
        response.append(s)
    generate(format_input(north_south_text=north_south, east_west_text=east_west), on_streamed_text=on_streamed_text, on_done=response.done_event.set)
    # sim_ai(on_streamed_text=on_streamed_text, on_done=response.done_event.set)

if __name__ == "__main__":
    response = Response(on_change=lambda: None)

    def on_streamed_text(s: str):
        response.append(s)
        print(response.parse())
    # generate(format_input("A few slow rhinos", "Many silly cats"), on_streamed_text=on_streamed_text, on_done=lambda: print(response.text))
    sim_ai(on_streamed_text=on_streamed_text, on_done=lambda: print(response._text))


