# gui_app/main_window.py
from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                            QTabWidget, QStatusBar, QMessageBox, QPushButton,
                            QLabel, QLineEdit, QTextEdit, QFileDialog,
                            QSpinBox, QComboBox, QProgressBar)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QFont, QPixmap, QImage
import matplotlib.pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
import numpy as np
from calculator_widget import CalculatorWidget
from image_processor_widget import ImageProcessorWidget

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.backend_thread = None
        self.init_ui()
    
    def init_ui(self):
        self.setWindowTitle("C++后端 + Python GUI应用")
        self.setGeometry(100, 100, 1200, 800)
        
        # 创建中心部件和布局
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # 创建标签页
        self.tab_widget = QTabWidget()
        main_layout.addWidget(self.tab_widget)
        
        # 添加各个功能标签页
        self.calculator_tab = CalculatorWidget()
        self.image_processor_tab = ImageProcessorWidget()
        self.plot_tab = PlotWidget()
        self.settings_tab = SettingsWidget()
        
        self.tab_widget.addTab(self.calculator_tab, "计算器")
        self.tab_widget.addTab(self.image_processor_tab, "图像处理")
        self.tab_widget.addTab(self.plot_tab, "数据可视化")
        self.tab_widget.addTab(self.settings_tab, "设置")
        
        # 状态栏
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("就绪")
        
        # 进度条
        self.progress_bar = QProgressBar()
        self.status_bar.addPermanentWidget(self.progress_bar)
        self.progress_bar.hide()
    
    def set_backend(self, backend_thread):
        """设置后端线程"""
        self.backend_thread = backend_thread
        self.calculator_tab.set_backend(backend_thread)
        self.image_processor_tab.set_backend(backend_thread)
        self.plot_tab.set_backend(backend_thread)
    
    def show_status(self, message, timeout=3000):
        """显示状态信息"""
        self.status_bar.showMessage(message, timeout)
    
    def show_progress(self, show=True, value=0):
        """显示/隐藏进度条"""
        if show:
            self.progress_bar.show()
            self.progress_bar.setValue(value)
        else:
            self.progress_bar.hide()