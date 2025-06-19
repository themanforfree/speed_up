use std::sync::Arc;

use pyo3::prelude::*;

#[pyclass]
struct Client {
    client: Arc<reqwest::Client>,
}

#[pymethods]
impl Client {
    #[new]
    fn new() -> Self {
        Client {
            client: Arc::new(reqwest::Client::new()),
        }
    }

    fn get<'py>(&self, py: Python<'py>, url: String) -> PyResult<Bound<'py, PyAny>> {
        let c = Arc::clone(&self.client);
        pyo3_async_runtimes::tokio::future_into_py(py, async move {
            let response = c.get(&url).send().await.unwrap();
            Ok(response.status().as_u16())
        })
    }
}

#[pymodule]
fn mini_requests_rust(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<Client>()?;
    Ok(())
}
