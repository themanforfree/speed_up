from collections.abc import Awaitable, Callable
import requests
import time
import math
from concurrent.futures import ThreadPoolExecutor, as_completed
import aiohttp
import asyncio

THREADS = 10
REQUESTS_PER_THREAD = 100
URL = "http://127.0.0.1:8000"


def send_req_requests(url: str) -> int:
    try:
        with requests.Session() as session:
            response = session.get(url)
            return response.status_code
    except requests.RequestException:
        return 0


async def send_req_aiohttp(url: str) -> int:
    try:
        async with aiohttp.ClientSession() as session:
            async with session.get(url) as response:
                return response.status
    except aiohttp.ClientError:
        return 0

async def send_req_reqwest_pyo3(url: str) -> int:
    try:
        import mini_requests_rust
        return await mini_requests_rust.send_req_reqwest(url)
    except Exception as e:
        return 0


def sync_thread(func):
    times = []
    for _ in range(REQUESTS_PER_THREAD):
        start = time.perf_counter_ns()
        status = func(URL)
        end = time.perf_counter_ns()
        assert status == 200, f"Request failed, status code: {status}"
        times.append((end - start) // 1000)
    return times


async def async_task(func):
    times = []
    for _ in range(REQUESTS_PER_THREAD):
        start = time.perf_counter_ns()
        status = await func(URL)
        end = time.perf_counter_ns()
        assert status == 200, f"Request failed, status code: {status}"
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


def sync_bench(func: Callable[[str], bool], lib_name: str):
    with ThreadPoolExecutor(max_workers=THREADS) as executor:
        futures = [executor.submit(sync_thread, func) for _ in range(THREADS)]
        all_times = []
        for future in as_completed(futures):
            times = future.result()
            all_times.extend(times)
    process_and_print_results(all_times,  lib_name)

async def async_bench(func: Callable[[str], Awaitable[bool]], lib_name: str):
    tasks = [async_task(func) for _ in range(THREADS)]
    all_times = await asyncio.gather(*tasks)
    all_times = [time for sublist in all_times for time in sublist]  # Flatten the list
    process_and_print_results(all_times, lib_name)

if __name__ == "__main__":
    sync_bench(send_req_requests, "requests")
    asyncio.run(async_bench(send_req_aiohttp, "aiohttp"))
    asyncio.run(async_bench(send_req_reqwest_pyo3, "reqwest"))
    pass