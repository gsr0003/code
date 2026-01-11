from PyQt6 import QtCore, QtGui, QtWidgets
class SettingsDialog(QtWidgets.QDialog):
    """设置对话框"""
    
    def __init__(self, parent=None, config_manager=None, main_ui=None):
        super().__init__(parent)
        self.config_manager = config_manager or ConfigManager()
        self.main_ui = main_ui
        self.initUI()
        self.load_settings()
        
    def initUI(self):
        """初始化UI"""
        self.setWindowTitle("系统设置")
        self.setFixedSize(500, 600)
        
        # 创建主布局
        main_layout = QtWidgets.QVBoxLayout(self)
        
        # 创建选项卡
        self.tab_widget = QtWidgets.QTabWidget()
        
        # 系统设置选项卡
        self.system_tab = QtWidgets.QWidget()
        self.initSystemTab()
        self.tab_widget.addTab(self.system_tab, "系统设置")
        
        # 备份设置选项卡
        self.backup_tab = QtWidgets.QWidget()
        self.initBackupTab()
        self.tab_widget.addTab(self.backup_tab, "备份设置")
        
        main_layout.addWidget(self.tab_widget)

        # 界面设置选项卡
        self.ui_tab = QtWidgets.QWidget()
        self.initUITab()
        self.tab_widget.addTab(self.ui_tab, "界面设置")
        
        # 按钮布局
        button_layout = QtWidgets.QHBoxLayout()
        
        self.save_button = QtWidgets.QPushButton("保存设置")
        self.save_button.clicked.connect(self.save_settings)
        
        self.cancel_button = QtWidgets.QPushButton("取消")
        self.cancel_button.clicked.connect(self.reject)
        
        self.reset_button = QtWidgets.QPushButton("恢复默认")
        self.reset_button.clicked.connect(self.reset_to_default)
        
        button_layout.addWidget(self.reset_button)
        button_layout.addStretch()
        button_layout.addWidget(self.save_button)
        button_layout.addWidget(self.cancel_button)
        
        main_layout.addLayout(button_layout)
    
    def initSystemTab(self):
        """初始化系统设置选项卡"""
        layout = QtWidgets.QFormLayout(self.system_tab)
        
        # 初始目录
        self.initial_dir_edit = QtWidgets.QLineEdit()
        self.browse_dir_button = QtWidgets.QPushButton("浏览...")
        self.browse_dir_button.clicked.connect(self.browse_initial_dir)
        
        dir_layout = QtWidgets.QHBoxLayout()
        dir_layout.addWidget(self.initial_dir_edit)
        dir_layout.addWidget(self.browse_dir_button)
        
        layout.addRow("初始目录:", dir_layout)
        
        # 页面大小
        self.page_size_spin = QtWidgets.QSpinBox()
        self.page_size_spin.setRange(5, 20)
        layout.addRow("每页显示文件数:", self.page_size_spin)
        
        # 启用备份
        self.backup_enabled_check = QtWidgets.QCheckBox("启用自动备份")
        layout.addRow(self.backup_enabled_check)
        
        # 启用云备份
        self.cloud_backup_check = QtWidgets.QCheckBox("启用云备份")
        layout.addRow(self.cloud_backup_check)
    
    def initBackupTab(self):
        """初始化备份设置选项卡"""
        layout = QtWidgets.QFormLayout(self.backup_tab)
        
        # 本地备份间隔
        self.local_interval_spin = QtWidgets.QSpinBox()
        self.local_interval_spin.setRange(5, 3600)
        self.local_interval_spin.setSuffix(" 秒")
        layout.addRow("本地备份间隔:", self.local_interval_spin)
        
        # 云备份间隔
        self.cloud_interval_spin = QtWidgets.QSpinBox()
        self.cloud_interval_spin.setRange(10, 7200)
        self.cloud_interval_spin.setSuffix(" 秒")
        layout.addRow("云备份间隔:", self.cloud_interval_spin)
        
        
        # 备份目录
        self.backup_dir_edit = QtWidgets.QLineEdit()
        self.browse_backup_button = QtWidgets.QPushButton("浏览...")
        self.browse_backup_button.clicked.connect(self.browse_backup_dir)
        
        backup_dir_layout = QtWidgets.QHBoxLayout()
        backup_dir_layout.addWidget(self.backup_dir_edit)
        backup_dir_layout.addWidget(self.browse_backup_button)
        
        layout.addRow("备份目录:", backup_dir_layout)

    def initUITab(self):
        """初始化界面设置选项卡"""
        layout = QtWidgets.QFormLayout(self.ui_tab)
        
        # 主题选择
        self.theme_combo = QtWidgets.QComboBox()
        self.theme_combo.addItems(["默认", "深色", "浅色"])
        layout.addRow("主题:", self.theme_combo)
        
        # 字体大小
        self.font_size_spin = QtWidgets.QSpinBox()
        self.font_size_spin.setRange(8, 20)
        layout.addRow("字体大小:", self.font_size_spin)
        
        # 语言选择
        self.language_combo = QtWidgets.QComboBox()
        self.language_combo.addItems(["中文", "English"])
        layout.addRow("语言:", self.language_combo)

        # 预览区域
        preview_label = QtWidgets.QLabel("设置将在重启后生效")
        preview_label.setStyleSheet("color: blue; font-style: italic;")
        layout.addRow("", preview_label)
    
    def browse_initial_dir(self):
        """浏览初始目录"""
        directory = QtWidgets.QFileDialog.getExistingDirectory(
            self, "选择初始目录", self.initial_dir_edit.text()
        )
        if directory:
            self.initial_dir_edit.setText(directory)
    
    def browse_backup_dir(self):
        """浏览备份目录"""
        directory = QtWidgets.QFileDialog.getExistingDirectory(
            self, "选择备份目录", self.backup_dir_edit.text()
        )
        if directory:
            self.backup_dir_edit.setText(directory)
    
    def load_settings(self):
        """加载当前设置到UI"""
        # 系统设置
        self.initial_dir_edit.setText(
            self.config_manager.get("system", "initial_directory")
        )
        self.page_size_spin.setValue(
            self.config_manager.get("system", "page_size")
        )
        self.backup_enabled_check.setChecked(
            self.config_manager.get("system", "backup_enabled")
        )
        self.cloud_backup_check.setChecked(
            self.config_manager.get("system", "cloud_backup_enabled")
        )
        
        # 备份设置
        self.local_interval_spin.setValue(
            self.config_manager.get("backup", "local_interval")
        )
        self.cloud_interval_spin.setValue(
            self.config_manager.get("backup", "cloud_interval")
        )
        self.backup_dir_edit.setText(
            self.config_manager.get("backup", "backup_dir")
        )
        
       
    
    def save_settings(self):
        """保存设置"""
        try:
            # 系统设置
            self.config_manager.set(
                "system", "initial_directory", 
                self.initial_dir_edit.text()
            )
            self.config_manager.set(
                "system", "page_size", 
                self.page_size_spin.value()
            )
            self.config_manager.set(
                "system", "backup_enabled",
                self.backup_enabled_check.isChecked()
            )
            self.config_manager.set(
                "system", "cloud_backup_enabled",
                self.cloud_backup_check.isChecked()
            )
            
            # 备份设置
            self.config_manager.set(
                "backup", "local_interval",
                self.local_interval_spin.value()
            )
            self.config_manager.set(
                "backup", "cloud_interval",
                self.cloud_interval_spin.value()
            )
            self.config_manager.set(
                "backup", "backup_dir",
                self.backup_dir_edit.text()
            )
            
            # 界面设置
            theme_map = {"默认": "default", "深色": "dark", "浅色": "light"}
            self.config_manager.set(
                "ui", "theme",
                theme_map.get(self.theme_combo.currentText(), "default")
            )
            
            self.config_manager.set(
                "ui", "font_size",
                self.font_size_spin.value()
            )
            
            lang_map = {"中文": "zh_CN", "English": "en_US"}
            self.config_manager.set(
                "ui", "language",
                lang_map.get(self.language_combo.currentText(), "zh_CN")
            )
            
            # 保存到文件
            if self.config_manager.save_config():
                # 更新主界面的参数
                if self.main_ui:
                    self.main_ui.path = self.initial_dir_edit.text()
                    self.main_ui.size = self.page_size_spin.value()
                    
                    # 更新备份线程间隔
                    if hasattr(self.main_ui, 'backup_local_thread'):
                        self.main_ui.backup_local_thread.interval = self.local_interval_spin.value()
                    if hasattr(self.main_ui, 'backup_online_thread'):
                        self.main_ui.backup_online_thread.interval = self.cloud_interval_spin.value()
                
                QtWidgets.QMessageBox.information(
                    self, "保存成功", "设置已保存成功！"
                )
                self.accept()
            else:
                QtWidgets.QMessageBox.warning(
                    self, "保存失败", "保存设置失败，请检查文件权限。"
                )
                
        except Exception as e:
            QtWidgets.QMessageBox.critical(
                self, "保存错误", f"保存设置时发生错误：{str(e)}"
            )
    
    def reset_to_default(self):
        """恢复默认设置"""
        reply = QtWidgets.QMessageBox.question(
            self, "确认恢复",
            "确定要恢复所有默认设置吗？当前设置将会丢失。",
            QtWidgets.QMessageBox.StandardButton.Yes | QtWidgets.QMessageBox.StandardButton.No,
            QtWidgets.QMessageBox.StandardButton.No
        )
        
        if reply == QtWidgets.QMessageBox.StandardButton.Yes:
            self.config_manager.config = ConfigManager.DEFAULT_CONFIG.copy()
            self.load_settings()