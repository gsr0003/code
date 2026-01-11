# 新增文件：config_manager.py
import json
import os
from pathlib import Path

class ConfigManager:
    """配置管理器"""
    
    DEFAULT_CONFIG = {
        "system": {
            "initial_directory": "C:/",
            "page_size": 8,
            "backup_enabled": True,
            "cloud_backup_enabled": True
        },
        "backup": {
            "local_interval": 10,  # 秒
            "cloud_interval": 10,   # 秒
            "backup_dir": "C:/FileBackup"
        },
        "ui": {
            "theme": "default",       # default, dark, light
            "font_size": 10,
            "language": "zh_CN"       # zh_CN, en_US
        }
    }
    
    def __init__(self, config_file="file_manager_config.json"):
        self.config_file = config_file
        self.config = self.load_config()
    
    def load_config(self):
        """加载配置文件"""
        try:
            if os.path.exists(self.config_file):
                with open(self.config_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
        except Exception as e:
            print(f"加载配置文件失败: {e}")
        
        # 如果文件不存在或读取失败，返回默认配置
        return self.DEFAULT_CONFIG.copy()
    
    def save_config(self):
        """保存配置文件"""
        try:
            with open(self.config_file, 'w', encoding='utf-8') as f:
                json.dump(self.config, f, indent=4, ensure_ascii=False)
            return True
        except Exception as e:
            print(f"保存配置文件失败: {e}")
            return False
    
    def get(self, section, key=None):
        """获取配置值"""
        if key:
            return self.config.get(section, {}).get(key)
        return self.config.get(section)
    
    def set(self, section, key, value):
        """设置配置值"""
        if section not in self.config:
            self.config[section] = {}
        self.config[section][key] = value
    
    def update_backup_threads(self, local_thread, cloud_thread):
        """更新备份线程的间隔时间"""
        if local_thread:
            local_thread.interval = self.get("backup", "local_interval")
        if cloud_thread:
            cloud_thread.interval = self.get("backup", "cloud_interval")