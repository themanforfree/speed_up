#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include <vector>
#include <cassert>
#include <future>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <chrono>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

const int THREADS = 10;
const int REQUESTS_PER_THREAD = 100;
const std::string HOST = "127.0.0.1";
const std::string PORT = "8000";
const std::string TARGET = "/";

unsigned send_req_beast(const std::string &host, const std::string &port, const std::string &target)
{
    try
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);
        beast::tcp_stream stream(ioc);
        stream.connect(results);

        http::request<http::empty_body> req{http::verb::get, target, 11};
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return static_cast<unsigned>(res.result_int());
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

std::vector<long long> thread_beast()
{

    std::vector<long long> times;
    times.reserve(REQUESTS_PER_THREAD);
    for (int i = 0; i < REQUESTS_PER_THREAD; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto status = send_req_beast(HOST, PORT, TARGET);
        auto end = std::chrono::high_resolution_clock::now();
        assert(status == 200);
        times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    }
    return times;
}

void process_and_print_results(std::vector<long long> &all_times)
{
    assert(all_times.size() == THREADS * REQUESTS_PER_THREAD);
    std::sort(all_times.begin(), all_times.end());
    double mean = std::accumulate(all_times.begin(), all_times.end(), 0LL) / static_cast<double>(all_times.size()) / 1000.0; // Convert to milliseconds
    size_t p99_index = static_cast<size_t>(std::ceil(all_times.size() * 0.99)) - 1;
    double p99 = all_times[p99_index] / 1000.0;
    size_t p999_index = static_cast<size_t>(std::ceil(all_times.size() * 0.999)) - 1;
    double p999 = all_times[p999_index] / 1000.0;
    double max_time = all_times.back() / 1000.0;

    std::cout << std::right << std::fixed << std::setprecision(2)
              << std::setw(10) << "C++" << "\t"
              << std::setw(10) << "beast" << "\t"
              << std::setw(8) << mean << "\t"
              << std::setw(8) << p99 << "\t"
              << std::setw(8) << p999 << "\t"
              << std::setw(8) << max_time
              << std::endl;
}

void bench_beast()
{
    std::vector<std::future<std::vector<long long>>> futures;
    for (int i = 0; i < THREADS; ++i)
    {
        futures.push_back(std::async(std::launch::async, thread_beast));
    }

    std::vector<long long> all_times;
    for (auto &future : futures)
    {
        auto times = future.get();
        all_times.insert(all_times.end(), times.begin(), times.end());
    }
    process_and_print_results(all_times);
}

int main()
{
    bench_beast();
}