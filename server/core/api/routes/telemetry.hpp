#pragma once
#include "third_party/httplib/httplib.h"

void handle_environment_report(const httplib::Request& req, httplib::Response& res);
void handle_module_report     (const httplib::Request& req, httplib::Response& res);
void handle_gameplay_report   (const httplib::Request& req, httplib::Response& res);