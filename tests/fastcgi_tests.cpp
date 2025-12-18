/*
 * FastCGI Integration Tests
 * Compile: c++ -std=c++98 -Wall -Wextra -Werror tests/fastcgi_tests.cpp -o tests/fastcgi_tests
 * Run: ./tests/fastcgi_tests
 */

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>

const int TEST_PORT = 8080;
const std::string TEST_HOST = "127.0.0.1";

int passed = 0;
int failed = 0;

std::string send_http_request(const std::string &request)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return "__SOCKET_FAIL__";
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    if (inet_pton(AF_INET, TEST_HOST.c_str(), &addr.sin_addr) != 1)
    {
        close(sock);
        std::cerr << "Invalid address" << std::endl;
        return "__ADDR_FAIL__";
    }

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        close(sock);
        std::cerr << "Connection failed - is server running?" << std::endl;
        return "__CONNECT_FAIL__";
    }

    if (send(sock, request.c_str(), request.size(), 0) < 0)
    {
        close(sock);
        std::cerr << "Send failed" << std::endl;
        return "__SEND_FAIL__";
    }

    std::string response;
    char buffer[4096];
    ssize_t n;
    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[n] = '\0';
        response.append(buffer);
        // Check if we got the full response
        if (response.find("\r\n\r\n") != std::string::npos)
        {
            // For chunked or simple responses, break after reasonable data
            if (response.find("Content-Length:") != std::string::npos ||
                response.find("Connection: close") != std::string::npos)
            {
                break;
            }
        }
    }

    close(sock);
    return response;
}

bool contains(const std::string &haystack, const std::string &needle)
{
    return haystack.find(needle) != std::string::npos;
}

void test_result(const std::string &test_name, bool success, const std::string &reason = "")
{
    if (success)
    {
        std::cout << "✓ " << test_name << std::endl;
        passed++;
    }
    else
    {
        std::cout << "✗ " << test_name;
        if (!reason.empty())
            std::cout << " - " << reason;
        std::cout << std::endl;
        failed++;
    }
}

void test_fastcgi_get()
{
    std::string request = 
        "GET /cgi-bin/test_get.py?param=value HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI GET: Returns 200 OK", 
                contains(response, "200 OK") || contains(response, "HTTP/1.1 200"),
                "Status line not found");
    
    test_result("FastCGI GET: Has correct content type",
                contains(response, "Content-Type: text/plain"),
                "Content-Type header missing");
    
    test_result("FastCGI GET: Contains expected output",
                contains(response, "FastCGI GET Test") || contains(response, "GET request successful"),
                "Expected output not found");
    
    test_result("FastCGI GET: Query string passed",
                contains(response, "param=value") || contains(response, "QUERY_STRING"),
                "Query string not passed to CGI");
}

void test_fastcgi_post()
{
    std::string body = "test_data=hello_world&key=value";
    std::ostringstream oss;
    oss << body.size();
    
    std::string request = 
        "POST /cgi-bin/test_post.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: " + oss.str() + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI POST: Returns 200 OK",
                contains(response, "200 OK") || contains(response, "HTTP/1.1 200"),
                "Status line not found");
    
    test_result("FastCGI POST: Body received",
                contains(response, "test_data=hello_world") || contains(response, "POST data"),
                "POST data not received by CGI");
    
    test_result("FastCGI POST: Content-Length passed",
                contains(response, "CONTENT_LENGTH:") || contains(response, "Content-Length:"),
                "Content-Length not passed");
}

void test_fastcgi_status_codes()
{
    // Test custom 404
    std::string request = 
        "GET /cgi-bin/test_status.py?code=404 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI Status: Custom 404",
                contains(response, "404"),
                "Custom 404 status not returned");
    
    // Test custom 500
    request = 
        "GET /cgi-bin/test_status.py?code=500 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    response = send_http_request(request);
    
    test_result("FastCGI Status: Custom 500",
                contains(response, "500"),
                "Custom 500 status not returned");
}

void test_fastcgi_headers()
{
    std::string request = 
        "GET /cgi-bin/test_headers.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI Headers: JSON content type",
                contains(response, "Content-Type: application/json"),
                "JSON content type not set");
    
    test_result("FastCGI Headers: Custom header",
                contains(response, "X-Custom-Header:"),
                "Custom header not present");
    
    test_result("FastCGI Headers: JSON body",
                contains(response, "{") && contains(response, "}"),
                "JSON body not found");
}

void test_fastcgi_html()
{
    std::string request = 
        "GET /cgi-bin/test_html.py?test=html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI HTML: Returns 200 OK",
                contains(response, "200 OK") || contains(response, "HTTP/1.1 200"),
                "Status line not found");
    
    test_result("FastCGI HTML: HTML content type",
                contains(response, "Content-Type: text/html"),
                "HTML content type not set");
    
    test_result("FastCGI HTML: Contains HTML tags",
                contains(response, "<html>") && contains(response, "</html>"),
                "HTML tags not found");
}

void test_fastcgi_echo()
{
    std::string body = "Echo this message back!";
    std::ostringstream oss;
    oss << body.size();
    
    std::string request = 
        "POST /cgi-bin/test_echo.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + oss.str() + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI Echo: Returns 200 OK",
                contains(response, "200 OK") || contains(response, "HTTP/1.1 200"),
                "Status line not found");
    
    test_result("FastCGI Echo: Message echoed back",
                contains(response, "Echo this message back!"),
                "Echo message not found in response");
}

void test_fastcgi_not_found()
{
    std::string request = 
        "GET /cgi-bin/nonexistent.py HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    std::string response = send_http_request(request);
    
    test_result("FastCGI Error: 404 for missing script",
                contains(response, "404") || contains(response, "Not Found"),
                "Should return 404 for missing script");
}

int main()
{
    std::cout << "\n==================================" << std::endl;
    std::cout << "FastCGI Integration Tests" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Prerequisites:" << std::endl;
    std::cout << "1. Server running on " << TEST_HOST << ":" << TEST_PORT << std::endl;
    std::cout << "2. FastCGI backend (python-flup) running on port 9000" << std::endl;
    std::cout << "3. Test scripts in cgi-bin/ directory" << std::endl;
    std::cout << "==================================\n" << std::endl;
    
    sleep(1);
    
    std::cout << "Running tests...\n" << std::endl;
    
    test_fastcgi_get();
    std::cout << std::endl;
    
    test_fastcgi_post();
    std::cout << std::endl;
    
    test_fastcgi_status_codes();
    std::cout << std::endl;
    
    test_fastcgi_headers();
    std::cout << std::endl;
    
    test_fastcgi_html();
    std::cout << std::endl;
    
    test_fastcgi_echo();
    std::cout << std::endl;
    
    test_fastcgi_not_found();
    std::cout << std::endl;
    
    std::cout << "==================================" << std::endl;
    std::cout << "Test Results:" << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;
    std::cout << "  Total:  " << (passed + failed) << std::endl;
    std::cout << "==================================" << std::endl;
    
    return failed > 0 ? 1 : 0;
}
