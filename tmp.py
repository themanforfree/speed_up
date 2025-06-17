import mini_requests_rust
import asyncio

async def main():
    print("Starting the request...")
    await mini_requests_rust.send_req_reqwest("http://example.com")
    print("Request completed!")

if __name__ == "__main__":
    asyncio.run(main())
    print("All done!")

