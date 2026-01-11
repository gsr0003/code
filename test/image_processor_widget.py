# gui_app/widgets/image_processor_widget.py
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton,
                            QLabel, QFileDialog, QComboBox, QSlider,
                            QGroupBox, QSpinBox, QProgressBar)
from PyQt6.QtGui import QPixmap, QImage
from PyQt6.QtCore import Qt, pyqtSignal, QThread
import numpy as np
import cv2

class ImageProcessorWidget(QWidget):
    image_processed = pyqtSignal(np.ndarray)
    
    def __init__(self):
        super().__init__()
        self.current_image = None
        self.backend_thread = None
        self.init_ui()
    
    def init_ui(self):
        layout = QHBoxLayout()
        
        # 左侧：图像显示和控制面板
        left_panel = QVBoxLayout()
        
        # 图像显示区域
        self.image_label = QLabel()
        self.image_label.setMinimumSize(400, 300)
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.image_label.setStyleSheet("border: 1px solid #ccc; background: #f0f0f0;")
        left_panel.addWidget(self.image_label)
        
        # 控制按钮
        button_layout = QHBoxLayout()
        
        self.load_button = QPushButton("加载图像")
        self.load_button.clicked.connect(self.load_image)
        button_layout.addWidget(self.load_button)
        
        self.save_button = QPushButton("保存图像")
        self.save_button.clicked.connect(self.save_image)
        button_layout.addWidget(self.save_button)
        
        self.reset_button = QPushButton("重置")
        self.reset_button.clicked.connect(self.reset_image)
        button_layout.addWidget(self.reset_button)
        
        left_panel.addLayout(button_layout)
        
        # 右侧：处理选项
        right_panel = QVBoxLayout()
        
        # 滤镜选择
        filter_group = QGroupBox("滤镜")
        filter_layout = QVBoxLayout()
        
        self.filter_combo = QComboBox()
        self.filter_combo.addItems(["高斯模糊", "边缘检测", "锐化", "浮雕"])
        filter_layout.addWidget(self.filter_combo)
        
        self.apply_filter_button = QPushButton("应用滤镜")
        self.apply_filter_button.clicked.connect(self.apply_filter)
        filter_layout.addWidget(self.apply_filter_button)
        
        filter_group.setLayout(filter_layout)
        right_panel.addWidget(filter_group)
        
        # 调整选项
        adjust_group = QGroupBox("调整")
        adjust_layout = QVBoxLayout()
        
        # 亮度调整
        brightness_layout = QHBoxLayout()
        brightness_layout.addWidget(QLabel("亮度:"))
        self.brightness_slider = QSlider(Qt.Orientation.Horizontal)
        self.brightness_slider.setRange(-100, 100)
        self.brightness_slider.setValue(0)
        brightness_layout.addWidget(self.brightness_slider)
        
        self.brightness_value_label = QLabel("0")
        brightness_layout.addWidget(self.brightness_value_label)
        
        self.brightness_slider.valueChanged.connect(
            lambda v: self.brightness_value_label.setText(str(v)))
        
        adjust_layout.addLayout(brightness_layout)
        
        self.adjust_brightness_button = QPushButton("调整亮度")
        self.adjust_brightness_button.clicked.connect(self.adjust_brightness)
        adjust_layout.addWidget(self.adjust_brightness_button)
        
        # 灰度转换
        self.grayscale_button = QPushButton("转为灰度图")
        self.grayscale_button.clicked.connect(self.convert_to_grayscale)
        adjust_layout.addWidget(self.grayscale_button)
        
        adjust_group.setLayout(adjust_layout)
        right_panel.addWidget(adjust_group)
        
        # 大小调整
        resize_group = QGroupBox("调整大小")
        resize_layout = QVBoxLayout()
        
        size_layout = QHBoxLayout()
        size_layout.addWidget(QLabel("宽度:"))
        self.width_spinbox = QSpinBox()
        self.width_spinbox.setRange(1, 4000)
        self.width_spinbox.setValue(800)
        size_layout.addWidget(self.width_spinbox)
        
        size_layout.addWidget(QLabel("高度:"))
        self.height_spinbox = QSpinBox()
        self.height_spinbox.setRange(1, 4000)
        self.height_spinbox.setValue(600)
        size_layout.addWidget(self.height_spinbox)
        
        resize_layout.addLayout(size_layout)
        
        self.resize_button = QPushButton("调整大小")
        self.resize_button.clicked.connect(self.resize_image)
        resize_layout.addWidget(self.resize_button)
        
        resize_group.setLayout(resize_layout)
        right_panel.addWidget(resize_group)
        
        # 进度条
        self.progress_bar = QProgressBar()
        right_panel.addWidget(self.progress_bar)
        
        layout.addLayout(left_panel, 60)
        layout.addLayout(right_panel, 40)
        self.setLayout(layout)
    
    def set_backend(self, backend_thread):
        self.backend_thread = backend_thread
    
    def load_image(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "选择图像文件", "", 
            "图像文件 (*.png *.jpg *.jpeg *.bmp *.tiff)"
        )
        
        if file_path:
            self.progress_bar.setValue(30)
            
            # 使用C++后端加载图像
            self.backend_thread.set_task("process_image", file_path, "load")
            self.backend_thread.start()
            
            self.parent().parent().show_status(f"加载图像: {file_path}")
    
    def apply_filter(self):
        if self.current_image is None:
            return
        
        filter_type = self.filter_combo.currentText()
        filter_map = {
            "高斯模糊": "gaussian",
            "边缘检测": "edge",
            "锐化": "sharpen",
            "浮雕": "emboss"
        }
        
        self.progress_bar.setValue(50)
        
        # 调用C++后端应用滤镜
        self.backend_thread.set_task("process_image", 
                                   self.current_image_path,
                                   "filter",
                                   filter_type=filter_map.get(filter_type, "gaussian"))
        self.backend_thread.start()
    
    def display_image(self, image_data):
        """显示图像数据（从C++后端接收）"""
        # 将C++返回的图像数据转换为QPixmap
        height = image_data.height
        width = image_data.width
        channels = image_data.channels
        pixels = image_data.pixels
        
        # 转换为numpy数组
        img_array = np.array(pixels, dtype=np.uint8).reshape((height, width, channels))
        
        # 转换颜色空间（如果需要）
        if channels == 3:
            qimage = QImage(img_array.data, width, height, 
                           width * channels, QImage.Format.Format_RGB888)
        else:
            qimage = QImage(img_array.data, width, height, 
                           width, QImage.Format.Format_Grayscale8)
        
        pixmap = QPixmap.fromImage(qimage)
        scaled_pixmap = pixmap.scaled(self.image_label.size(), 
                                     Qt.AspectRatioMode.KeepAspectRatio,
                                     Qt.TransformationMode.SmoothTransformation)
        self.image_label.setPixmap(scaled_pixmap)
        self.progress_bar.setValue(100)