# To run this code you need to install the following dependencies:
# pip install google-genai

from google import genai
from dotenv import load_dotenv

load_dotenv()

def generate(input: str):
    client = genai.Client()
    stream = client.interactions.create(
        model="gemini-3.1-flash-lite",
        input=input,
        stream=True
    )
    for event in stream:
        if event.event_type == "step.delta":
            if event.delta.type == "text":
                print(event.delta.text, end="", flush=True)
if __name__ == "__main__":

    input = """ 
    I have a traffic light that I need help figuring out my traffic green durations for. Can you help me figure out ideal traffic conditions?

    North/South: 100 cars
    East/West: 10 cars
    
    Return two space separated integers (north/south green duration, east/west green duration) indicating the duration in seconds (in range >=1 and <=20)
    in the first line in the response, and then explain reasoning in the following lines.
    """ 
    generate(input)


