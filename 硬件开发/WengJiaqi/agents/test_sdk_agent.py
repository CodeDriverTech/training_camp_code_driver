from anthropic import Anthropic
from dotenv import load_dotenv
import os

load_dotenv()
#TODO
client = Anthropic(
    api_key=os.getenv("ANTHROPIC_API_KEY"),
    base_url=os.getenv("ANTHROPIC_API_URL")
)

resp = client.messages.create(
    model="claude-sonnet-4-5-20250929",
    messages=[{"role": "user", "content": "Hello"}],
    max_tokens=50
)

print(resp)