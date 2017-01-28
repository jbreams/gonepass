#pragma once
#include <memory>

class MainWindow;
MainWindow* getMainWindow();
void errorDialog(const std::string& msg);
std::string calculateTOTP(const std::string& uri);
bool isTOTPURI(const std::string& uri);
