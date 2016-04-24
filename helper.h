#pragma once
#include <memory>

class MainWindow;
MainWindow* getMainWindow();
void errorDialog(const std::string msg);
