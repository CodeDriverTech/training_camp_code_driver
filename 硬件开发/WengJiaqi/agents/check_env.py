from dotenv import load_dotenv
import os

load_dotenv()

print("API KEY =", os.getenv("ANTHROPIC_API_KEY"))
print("API URL =", os.getenv("ANTHROPIC_API_URL"))