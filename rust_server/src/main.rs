// use std::sync::atomic::AtomicUsize;

use axum::{Router, response::Html, routing::get};

// static COUNT: AtomicUsize = AtomicUsize::new(1);

async fn hello() -> Html<&'static str> {
    // let count = COUNT.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
    // if count % 100 == 0 {
    //     println!("Request count: {}", count);
    // }
    Html("<h1>Hello, Axum!</h1>")
}

#[tokio::main]
async fn main() {
    let app = Router::new().route("/", get(hello));
    let listener = tokio::net::TcpListener::bind("0.0.0.0:8000").await.unwrap();
    axum::serve(listener, app.into_make_service())
        .await
        .unwrap();
}
