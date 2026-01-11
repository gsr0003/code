// cpp_backend/src/bindings.cpp - pybind11绑定
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "Calculator.h"
#include "ImageProcessor.h"

namespace py = pybind11;

PYBIND11_MODULE(cpp_backend, m) {
    m.doc() = "C++高性能后端模块";
    
    // 绑定Calculator类
    py::class_<Backend::Calculator>(m, "Calculator")
        .def(py::init<>())
        .def("add", &Backend::Calculator::add, "加法运算")
        .def("subtract", &Backend::Calculator::subtract, "减法运算")
        .def("multiply", &Backend::Calculator::multiply, "乘法运算")
        .def("divide", &Backend::Calculator::divide, "除法运算")
        .def("calculate_expression", &Backend::Calculator::calculate_expression, 
             "计算表达式", py::arg("expr"))
        .def("solve_equation", &Backend::Calculator::solve_equation, 
             "解二次方程", py::arg("a"), py::arg("b"), py::arg("c"))
        .def("process_batch", &Backend::Calculator::process_batch, 
             "批量处理数据", py::arg("input"))
        .def("set_precision", &Backend::Calculator::set_precision, 
             "设置精度", py::arg("precision"))
        .def("get_precision", &Backend::Calculator::get_precision, 
             "获取精度")
        .def("clear_history", &Backend::Calculator::clear_history, 
             "清除历史记录")
        .def("get_history", &Backend::Calculator::get_history, 
             "获取历史记录");
    
    // 绑定ImageProcessor类和ImageData结构体
    py::class_<Backend::ImageProcessor::ImageData>(m, "ImageData")
        .def(py::init<>())
        .def_readwrite("width", &Backend::ImageProcessor::ImageData::width)
        .def_readwrite("height", &Backend::ImageProcessor::ImageData::height)
        .def_readwrite("channels", &Backend::ImageProcessor::ImageData::channels)
        .def_readwrite("pixels", &Backend::ImageProcessor::ImageData::pixels);
    
    py::class_<Backend::ImageProcessor>(m, "ImageProcessor")
        .def(py::init<>())
        .def("load_image", &Backend::ImageProcessor::load_image, 
             "加载图像", py::arg("path"))
        .def("save_image", &Backend::ImageProcessor::save_image, 
             "保存图像", py::arg("img"), py::arg("path"))
        .def("resize_image", &Backend::ImageProcessor::resize_image, 
             "调整图像大小", py::arg("img"), py::arg("new_width"), py::arg("new_height"))
        .def("apply_filter", &Backend::ImageProcessor::apply_filter, 
             "应用滤镜", py::arg("img"), py::arg("filter_type"))
        .def("adjust_brightness", &Backend::ImageProcessor::adjust_brightness, 
             "调整亮度", py::arg("img"), py::arg("delta"))
        .def("convert_to_grayscale", &Backend::ImageProcessor::convert_to_grayscale, 
             "转换为灰度图")
        .def("get_histogram", &Backend::ImageProcessor::get_histogram, 
             "获取直方图", py::arg("img"))
        .def("calculate_psnr", &Backend::ImageProcessor::calculate_psnr, 
             "计算PSNR", py::arg("img1"), py::arg("img2"));
}