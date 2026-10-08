#pragma once
#include "third_party/httplib/httplib.h"

void handle_get_status(const httplib::Request& req, httplib::Response& res);