use futures::future::join_all;
use reqwest::Client;
use std::time::Instant;
use tokio::task;

const THREADS: usize = 10;
const REQUESTS_PER_THREAD: usize = 100;
const URL: &str = "http://127.0.0.1:8000";

async fn thread_reqwest() -> (Vec<u128>, u128) {
    let client = Client::new();
    let mut times = Vec::with_capacity(REQUESTS_PER_THREAD);
    let loop_start = Instant::now();
    for _ in 0..REQUESTS_PER_THREAD {
        let start = Instant::now();
        let status = client
            .get(URL)
            .send()
            .await
            .map(|res| res.status().as_u16());
        let elapsed = start.elapsed().as_micros();
        assert!(status.is_ok_and(|r| r == 200));
        times.push(elapsed);
    }
    let loop_elapsed = loop_start.elapsed().as_micros();
    (times, loop_elapsed)
}

fn process_and_print_results(mut all_times: Vec<u128>, loop_times: Vec<u128>, bench_elapsed: u128) {
    assert_eq!(all_times.len(), THREADS * REQUESTS_PER_THREAD);
    all_times.sort_unstable();

    let mean = all_times.iter().copied().sum::<u128>() as f64 / all_times.len() as f64 / 1000.0;
    let p99_index = ((all_times.len() as f64) * 0.99).ceil() as usize - 1;
    let p99 = all_times[p99_index] as f64 / 1000.0;
    let p999_index = ((all_times.len() as f64) * 0.999).ceil() as usize - 1;
    let p999 = all_times[p999_index] as f64 / 1000.0;
    let max = *all_times.last().unwrap() as f64 / 1000.0;

    let loop_mean =
        loop_times.iter().copied().sum::<u128>() as f64 / loop_times.len() as f64 / 1000.0;
    let bench_time = bench_elapsed as f64 / 1000.0;

    println!(
        "{:>10}\t{:>10}\t{:>8.2}\t{:>8.2}\t{:>8.2}\t{:>8.2}\t{:>8.2}\t{:>8.2}",
        "Rust", "reqwest", mean, p99, p999, max, loop_mean, bench_time
    );
}

async fn bench_reqwest() {
    let bench_start = Instant::now();
    let handles = (0..THREADS)
        .map(|_| task::spawn(thread_reqwest()))
        .collect::<Vec<_>>();
    let mut all_times = vec![];
    let mut loop_times = vec![];
    for r in join_all(handles).await {
        let (times, loop_elapsed) = r.unwrap();
        all_times.extend(times);
        loop_times.push(loop_elapsed);
    }
    let bench_elapsed = bench_start.elapsed().as_micros();
    process_and_print_results(all_times, loop_times, bench_elapsed);
}

#[tokio::main(flavor = "multi_thread", worker_threads = 10)]
async fn main() {
    bench_reqwest().await;
}
