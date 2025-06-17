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

unsigned send_req_beast(const std::string &url)
{
    try
    {
        boost::urls::url_view u = boost::urls::parse_uri(url).value();
        std::string host = u.host();
        std::string port = u.has_port() ? u.port() : (u.scheme() == "https" ? "443" : "80");
        std::string path = u.encoded_path().empty() ? "/" : std::string(u.encoded_path());

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);
        beast::tcp_stream stream(ioc);
        stream.connect(results);

        http::request<http::empty_body> req{http::verb::get, path, 11};
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.result_int();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0;
    }
}

// 使用Boost.Python导出函数
BOOST_PYTHON_MODULE(mini_requests_cpp)
{
    using namespace boost::python;
    def("send_req_beast", send_req_beast, "A function which sends a GET request using Boost.Beast");
}
