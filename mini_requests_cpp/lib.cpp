#include <boost/python.hpp>

// 简单的加法函数
int add(int a, int b)
{
    return a + b;
}

// 使用Boost.Python导出函数
BOOST_PYTHON_MODULE(mini_requests_cpp)
{
    using namespace boost::python;
    def("add", add, "A function which adds two numbers");
}
