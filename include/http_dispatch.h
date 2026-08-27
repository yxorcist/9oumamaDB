#ifndef HTTP_DISPATCH_H
#define HTTP_DISPATCH_H

#include "http_request.h"
#include "request_queue.h"

int http_request_to_db_request(const HTTPRequest *http_request, Request *request);

#endif

