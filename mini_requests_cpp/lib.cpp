#include <boost/python.hpp>
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

struct ParsedURL
{
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
};

ParsedURL parse_url(const std::string &url)
{
    ParsedURL result;
    std::size_t pos = 0;

    // 1. 解析 scheme（直到 "://"）
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos)
        throw std::invalid_argument("Invalid URL: missing scheme");
    result.scheme = url.substr(0, scheme_end);
    pos = scheme_end + 3; // 跳过 "://"

    // 2. host[:port] 起始位置
    std::size_t host_start = pos;

    // 3. 终止 host[:port] 的位置是 '/' 或 '?' 或 '#' 或字符串末尾
    std::size_t path_start = url.find_first_of("/?#", pos);
    std::size_t authority_end = (path_start == std::string::npos) ? url.length() : path_start;

    // 4. host[:port]
    std::string host_port = url.substr(host_start, authority_end - host_start);
    auto colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos)
    {
        result.host = host_port.substr(0, colon_pos);
        result.port = host_port.substr(colon_pos + 1);
    }
    else
    {
        result.host = host_port;
        result.port = (result.scheme == "https") ? "443" : "80"; // 默认端口
    }

    // 5. target（包括 path + query + fragment）
    if (path_start != std::string::npos)
    {
        result.target = url.substr(path_start);
    }
    else
    {
        result.target = "/";
    }

    return result;
}

class AllowThreads
{
public:
    AllowThreads() { _save = PyEval_SaveThread(); }
    ~AllowThreads() { PyEval_RestoreThread(_save); }

private:
    PyThreadState *_save;
};

class Client
{
private:
    net::io_context ioc_;
    std::string current_host_;
    std::string current_port_;
    std::unique_ptr<tcp::resolver> resolver_;
    std::unique_ptr<beast::tcp_stream> stream_;
    bool connected_ = false;

    // 初始化与指定主机的连接
    bool init_connection(const std::string &host, const std::string &port)
    {
        try
        {
            // 如果已连接到其他主机，则先关闭连接
            if (connected_ && (host != current_host_ || port != current_port_))
            {
                close();
            }

            // 如果未连接，建立新连接
            if (!connected_)
            {
                if (!resolver_)
                {
                    resolver_ = std::make_unique<tcp::resolver>(ioc_);
                }
                if (!stream_)
                {
                    stream_ = std::make_unique<beast::tcp_stream>(ioc_);
                }

                AllowThreads _allow_threads; // Allow Python threads to run during connection
                auto const results = resolver_->resolve(host, port);
                stream_->connect(results);

                current_host_ = host;
                current_port_ = port;
                connected_ = true;
            }
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Connection error: " << e.what() << std::endl;
            connected_ = false;
            return false;
        }
    }

public:
    Client() {}

    ~Client()
    {
        close();
    }

    // 关闭连接
    void close()
    {
        if (connected_ && stream_)
        {
            beast::error_code ec;
            stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
            connected_ = false;
        }
    }

    unsigned get(const std::string &url_str)
    {
        try
        {
            auto parsed_url = parse_url(url_str);

            // 确保连接已建立
            if (!init_connection(parsed_url.host, parsed_url.port))
            {
                return 0;
            }

            // 构建并发送请求
            AllowThreads _allowThreads;
            http::request<http::empty_body> req{http::verb::get, parsed_url.target, 11};
            req.set(http::field::host, parsed_url.host);
            http::write(*stream_, req);

            // 接收响应
            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(*stream_, buffer, res);
            return static_cast<unsigned>(res.result_int());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            connected_ = false; // 发生错误，标记连接已断开
            return 0;
        }
    }
};

// 使用Boost.Python导出函数
BOOST_PYTHON_MODULE(mini_requests_cpp)
{
    using namespace boost::python;
    class_<Client, boost::noncopyable>("Client")
        .def("get", &Client::get)
        .def("close", &Client::close);
}
