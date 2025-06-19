#include <boost/python.hpp>
#include <boost/url.hpp>
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
            // 解析URL
            boost::system::result<boost::urls::url_view> result = boost::urls::parse_uri(url_str);
            if (!result)
            {
                std::cerr << "Error: Invalid URL: " << result.error().message() << std::endl;
                return 0;
            }

            auto url = result.value();
            std::string host = url.host();
            std::string port = url.has_port() ? std::string(url.port()) : (url.scheme() == "https" ? "443" : "80");
            std::string target = url.path();
            if (target.empty())
                target = "/";
            if (!url.query().empty())
                target += "?" + std::string(url.query());

            // 确保连接已建立
            if (!init_connection(host, port))
            {
                return 0;
            }

            // 构建并发送请求
            http::request<http::empty_body> req{http::verb::get, target, 11};
            req.set(http::field::host, host);
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
