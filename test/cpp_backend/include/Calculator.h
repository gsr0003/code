 // cpp_backend/include/Calculator.h
#pragma once
#include <vector>
#include <string>

namespace Backend {
    class Calculator {
    public:
        Calculator();
        
        // 基本计算
        double add(double a, double b);
        double subtract(double a, double b);
        double multiply(double a, double b);
        double divide(double a, double b);
        
        // 高级计算
        double calculate_expression(const std::string& expr);
        std::vector<double> solve_equation(double a, double b, double c);
        
        // 批量处理
        std::vector<double> process_batch(const std::vector<double>& input);
        
        // 状态管理
        void set_precision(int precision);
        int get_precision() const;
        void clear_history();
        std::vector<std::string> get_history() const;
        
    private:
        int precision_;
        std::vector<std::string> calculation_history_;
    };

    class ImageProcessor {
    public:
        struct ImageData {
            int width;
            int height;
            int channels;
            std::vector<unsigned char> pixels;
        };
        
        ImageProcessor();
        
        // 图像处理操作
        ImageData load_image(const std::string& path);
        bool save_image(const ImageData& img, const std::string& path);
        ImageData resize_image(const ImageData& img, int new_width, int new_height);
        ImageData apply_filter(const ImageData& img, const std::string& filter_type);
        ImageData adjust_brightness(const ImageData& img, int delta);
        ImageData convert_to_grayscale(const ImageData& img);
        
        // 图像分析
        std::vector<double> get_histogram(const ImageData& img);
        double calculate_psnr(const ImageData& img1, const ImageData& img2);
        
    private:
        ImageData apply_gaussian_blur(const ImageData& img);
        ImageData apply_sobel_edge(const ImageData& img);
    };
}