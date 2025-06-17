use axum::{Router, response::Html, routing::get};

async fn hello() -> Html<&'static str> {
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
