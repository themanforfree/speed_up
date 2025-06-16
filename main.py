import requests
import time
import math
from concurrent.futures import ThreadPoolExecutor, as_completed
import aiohttp
import asyncio

THREADS = 10
REQUESTS_PER_THREAD = 100
URL = "http://127.0.0.1:8000"


def send_req_requests(url: str) -> bool:
    try:
        with requests.Session() as session:
            response = session.get(url)
            return response.status_code == 200
    except requests.RequestException:
        return False


async def send_req_aiohttp(url: str) -> bool:
    try:
        async with aiohttp.ClientSession() as session:
            async with session.get(url) as response:
                return response.status == 200
    except aiohttp.ClientError:
        return False


def thread_requests():
    times = []
    for _ in range(REQUESTS_PER_THREAD):
        start = time.perf_counter_ns()
        status = send_req_requests(URL)
        end = time.perf_counter_ns()
        assert status, "Request failed"
        times.append((end - start) // 1000)
    return times


async def thread_aiohttp():
    times = []
    for _ in range(REQUESTS_PER_THREAD):
        start = time.perf_counter_ns()
        status = await send_req_aiohttp(URL)
        end = time.perf_counter_ns()
        assert status, "Request failed"
        times.append((end - start) // 1000)
    return times


def process_and_print_results(all_times, lib_name):
    assert len(all_times) == THREADS * REQUESTS_PER_THREAD, "Not enough data collected"
    all_times.sort()
    mean = sum(all_times) / len(all_times) / 1000.0
    p99_index = int(math.ceil(len(all_times) * 0.99)) - 1
    p99 = all_times[p99_index] / 1000.0
    p999_index = int(math.ceil(len(all_times) * 0.999)) - 1
    p999 = all_times[p999_index] / 1000.0
    max_time = all_times[-1] / 1000.0
    print(
        f"{'Python':>10}\t{lib_name:>10}\t{mean:8.2f}\t{p99:8.2f}\t{p999:8.2f}\t{max_time:8.2f}"
    )


def bench_requests():
    with ThreadPoolExecutor(max_workers=THREADS) as executor:
        futures = [executor.submit(thread_requests) for _ in range(THREADS)]
        all_times = []
        for future in as_completed(futures):
            times = future.result()
            all_times.extend(times)
    process_and_print_results(all_times, "requests")


async def bench_aiohttp():
    tasks = [thread_aiohttp() for _ in range(THREADS)]
    all_times = await asyncio.gather(*tasks)
    all_times = [time for sublist in all_times for time in sublist]  # Flatten the list
    process_and_print_results(all_times, "aiohttp")


if __name__ == "__main__":
    bench_requests()
    asyncio.run(bench_aiohttp())
