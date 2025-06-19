import requests
import time
import math
from concurrent.futures import ThreadPoolExecutor, as_completed
import aiohttp
import asyncio
import mini_requests_rust
import mini_requests_cpp

THREADS = 10
REQUESTS_PER_THREAD = 100
URL = "http://127.0.0.1:8000"


class RequestsClient:
    def __init__(self):
        self.session = requests.Session()

    def get(self, url):
        try:
            response = self.session.get(url)
            return response.status_code
        except requests.RequestException:
            return 0

    def close(self):
        self.session.close()


class AiohttpClient:
    def __init__(self):
        self.session = aiohttp.ClientSession()

    async def get(self, url: str) -> int:
        try:
            async with self.session.get(url) as response:
                return response.status
        except aiohttp.ClientError:
            return 0

    async def close(self):
        await self.session.close()


class ReqwestClient:
    def __init__(self):
        self.session = mini_requests_rust.Client()

    def get(self, url: str) -> int:
        try:
            return self.session.get(url)
        except requests.RequestException:
            return 0

    async def close(self):
        pass


class BeastClient:
    def __init__(self):
        self.client = mini_requests_cpp.Client()

    def get(self, url: str) -> int:
        try:
            return self.client.get(url)
        except requests.RequestException:
            return 0

    def close(self):
        self.client.close()


def sync_thread(target_class):
    client = target_class()
    times = []
    loop_start = time.perf_counter_ns()
    for _ in range(REQUESTS_PER_THREAD):
        start = time.perf_counter_ns()
        status = client.get(URL)
        end = time.perf_counter_ns()
        assert status == 200, f"Request failed, status code: {status}"
        times.append((end - start) // 1000)
    loop_end = time.perf_counter_ns()
    loop_elapsed = (loop_end - loop_start) // 1000
    # print(f"Thread {target_class.__name__} completed in {loop_elapsed/1000.0:.2f} ms")
    client.close()
    return times, loop_elapsed


async def async_task(target_class):
    client = target_class()
    times = []
    loop_start = time.perf_counter_ns()
    for _ in range(REQUESTS_PER_THREAD):
        start = time.perf_counter_ns()
        status = await client.get(URL)
        end = time.perf_counter_ns()
        assert status == 200, f"Request failed, status code: {status}"
        times.append((end - start) // 1000)
    loop_end = time.perf_counter_ns()
    loop_elapsed = (loop_end - loop_start) // 1000
    # print(f"Async task {target_class.__name__} completed in {loop_elapsed/1000.0:.2f} ms")
    await client.close()
    return times, loop_elapsed


def process_and_print_results(all_times, loop_times, bench_elapsed, lib_name):
    assert len(all_times) == THREADS * REQUESTS_PER_THREAD, "Not enough data collected"
    all_times.sort()
    mean = sum(all_times) / len(all_times) / 1000.0
    p99_index = int(math.ceil(len(all_times) * 0.99)) - 1
    p99 = all_times[p99_index] / 1000.0
    p999_index = int(math.ceil(len(all_times) * 0.999)) - 1
    p999 = all_times[p999_index] / 1000.0
    max_time = all_times[-1] / 1000.0

    loop_mean = sum(loop_times) / len(loop_times) / 1000.0
    bench_time = bench_elapsed / 1000.0

    print(
        f"{'Python':>10}\t{lib_name:>10}\t{mean:8.2f}\t{p99:8.2f}\t{p999:8.2f}\t{max_time:8.2f}\t{loop_mean:8.2f}\t{bench_time:8.2f}"
    )


def sync_bench(target_class, lib_name: str):

    with ThreadPoolExecutor(max_workers=THREADS) as executor:
        bench_start = time.perf_counter_ns()
        futures = [executor.submit(sync_thread, target_class) for _ in range(THREADS)]
        all_times = []
        loop_times = []
        for future in as_completed(futures):
            times, loop_elapsed = future.result()
            all_times.extend(times)
            loop_times.append(loop_elapsed)
        bench_end = time.perf_counter_ns()
    process_and_print_results(
        all_times, loop_times, (bench_end - bench_start) // 1000, lib_name
    )


async def async_bench(target_class, lib_name: str):
    bench_start = time.perf_counter_ns()
    tasks = [async_task(target_class) for _ in range(THREADS)]
    all_times = []
    loop_times = []
    for times, loop_elapsed in await asyncio.gather(*tasks):
        all_times.extend(times)
        loop_times.append(loop_elapsed)
    bench_end = time.perf_counter_ns()
    process_and_print_results(
        all_times, loop_times, (bench_end - bench_start) // 1000, lib_name
    )


if __name__ == "__main__":
    sync_bench(RequestsClient, "requests")
    sync_bench(BeastClient, "beast")
    asyncio.run(async_bench(AiohttpClient, "aiohttp"))
    asyncio.run(async_bench(ReqwestClient, "reqwest"))
