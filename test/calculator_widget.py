 # gui_app/widgets/calculator_widget.py
from PyQt6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
                            QLabel, QLineEdit, QPushButton, QTextEdit,
                            QGroupBox, QComboBox)
from PyQt6.QtCore import Qt
import numpy as np

class CalculatorWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.backend_thread = None
        self.init_ui()
    
    def init_ui(self):
        layout = QVBoxLayout()
        
        # 表达式计算区域
        expr_group = QGroupBox("表达式计算")
        expr_layout = QVBoxLayout()
        
        self.expr_input = QLineEdit()
        self.expr_input.setPlaceholderText("输入数学表达式，如: 2+3*sin(45)")
        expr_layout.addWidget(self.expr_input)
        
        self.calc_button = QPushButton("计算")
        self.calc_button.clicked.connect(self.calculate_expression)
        expr_layout.addWidget(self.calc_button)
        
        self.result_label = QLabel("结果: ")
        expr_layout.addWidget(self.result_label)
        
        expr_group.setLayout(expr_layout)
        layout.addWidget(expr_group)
        
        # 方程求解区域
        eq_group = QGroupBox("方程求解")
        eq_layout = QGridLayout()
        
        eq_layout.addWidget(QLabel("a:"), 0, 0)
        self.a_input = QLineEdit("1")
        eq_layout.addWidget(self.a_input, 0, 1)
        
        eq_layout.addWidget(QLabel("b:"), 1, 0)
        self.b_input = QLineEdit("-3")
        eq_layout.addWidget(self.b_input, 1, 1)
        
        eq_layout.addWidget(QLabel("c:"), 2, 0)
        self.c_input = QLineEdit("2")
        eq_layout.addWidget(self.c_input, 2, 1)
        
        self.solve_button = QPushButton("解方程")
        self.solve_button.clicked.connect(self.solve_equation)
        eq_layout.addWidget(self.solve_button, 3, 0, 1, 2)
        
        self.eq_result_text = QTextEdit()
        self.eq_result_text.setMaximumHeight(100)
        eq_layout.addWidget(self.eq_result_text, 4, 0, 1, 2)
        
        eq_group.setLayout(eq_layout)
        layout.addWidget(eq_group)
        
        # 批量处理区域
        batch_group = QGroupBox("批量数据处理")
        batch_layout = QVBoxLayout()
        
        self.batch_input = QTextEdit()
        self.batch_input.setPlaceholderText("每行输入一个数字")
        batch_layout.addWidget(self.batch_input)
        
        self.process_batch_button = QPushButton("批量处理")
        self.process_batch_button.clicked.connect(self.process_batch)
        batch_layout.addWidget(self.process_batch_button)
        
        self.batch_result_text = QTextEdit()
        batch_layout.addWidget(self.batch_result_text)
        
        batch_group.setLayout(batch_layout)
        layout.addWidget(batch_group)
        
        self.setLayout(layout)
    
    def set_backend(self, backend_thread):
        self.backend_thread = backend_thread
    
    def calculate_expression(self):
        if not self.backend_thread:
            return
        
        expr = self.expr_input.text()
        if not expr:
            return
        
        self.backend_thread.set_task("calculate_expression", expr)
        self.backend_thread.start()
        self.parent().parent().show_progress(True, 50)
    
    def solve_equation(self):
        try:
            a = float(self.a_input.text())
            b = float(self.b_input.text())
            c = float(self.c_input.text())
            
            # 直接调用C++方法（如果计算量小）
            result = self.backend_thread.calculator.solve_equation(a, b, c)
            
            if result:
                self.eq_result_text.setText(f"解: {result}")
            else:
                self.eq_result_text.setText("无实根")
        except Exception as e:
            self.eq_result_text.setText(f"错误: {str(e)}")
    
    def process_batch(self):
        text = self.batch_input.toPlainText()
        try:
            numbers = [float(line.strip()) for line in text.split('\n') if line.strip()]
            result = self.backend_thread.calculator.process_batch(numbers)
            self.batch_result_text.setText("\n".join(f"{num:.6f}" for num in result))
        except Exception as e:
            self.batch_result_text.setText(f"错误: {str(e)}") 