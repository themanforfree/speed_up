use pyo3::prelude::*;
use reqwest::Client;

#[pyfunction]
fn send_req_reqwest(py: Python, url: String) -> PyResult<Bound<PyAny>> {
    pyo3_async_runtimes::tokio::future_into_py(py, async move {
        let client = Client::new();
        let response = client.get(url).send().await.unwrap();
        Ok(response.status().as_u16())
    })
}

#[pymodule]
fn mini_requests_rust(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_function(wrap_pyfunction!(send_req_reqwest, m)?)?;
    Ok(())
}
