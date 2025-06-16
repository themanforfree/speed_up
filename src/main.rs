use futures::future::join_all;
use reqwest::Client;
use std::time::Instant;
use tokio::task;

const THREADS: usize = 10;
const REQUESTS_PER_THREAD: usize = 100;
const URL: &str = "http://127.0.0.1:8000";

async fn send_req_reqwest(url: &str) -> Result<u16, reqwest::Error> {
    let client = Client::new();
    let response = client.get(url).send().await?;
    Ok(response.status().as_u16())
}

async fn thread_reqwest() -> Vec<u128> {
    let mut times = Vec::with_capacity(REQUESTS_PER_THREAD);
    for _ in 0..REQUESTS_PER_THREAD {
        let start = Instant::now();
        let status = send_req_reqwest(URL).await;
        let elapsed = start.elapsed().as_micros();
        assert!(status.is_ok_and(|r| r == 200));
        times.push(elapsed);
    }
    times
}

fn process_and_print_results(mut all_times: Vec<u128>) {
    assert_eq!(all_times.len(), THREADS * REQUESTS_PER_THREAD);
    all_times.sort_unstable();

    let mean = all_times.iter().copied().sum::<u128>() as f64 / all_times.len() as f64 / 1000.0;
    let p99_index = ((all_times.len() as f64) * 0.99).ceil() as usize - 1;
    let p99 = all_times[p99_index] as f64 / 1000.0;
    let p999_index = ((all_times.len() as f64) * 0.999).ceil() as usize - 1;
    let p999 = all_times[p999_index] as f64 / 1000.0;
    let max = *all_times.last().unwrap() as f64 / 1000.0;

    println!(
        "{:>10}\t{:>10}\t{:>8.2}\t{:>8.2}\t{:>8.2}\t{:>8.2}",
        "Rust", "reqwest", mean, p99, p999, max
    );
}

async fn bench_reqwest() {
    let handles = (0..THREADS)
        .map(|_| task::spawn(thread_reqwest()))
        .collect::<Vec<_>>();

    let all_times = join_all(handles)
        .await
        .into_iter()
        .map(|h| h.unwrap())
        .flatten()
        .collect::<Vec<_>>();
    process_and_print_results(all_times);
}

#[tokio::main(flavor = "multi_thread", worker_threads = 10)]
async fn main() {
    bench_reqwest().await;
}
