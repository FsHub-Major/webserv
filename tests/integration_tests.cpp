// integration_tests.cpp
// Unified test suite: basic HTTP, multi-client concurrency, stress, 404, invalid method
// Build separately from server (server uses -std=c++98). Tests use C++11.

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

static const int TEST_PORT = 8080;
static const char *TEST_HOST = "127.0.0.1";
static const int CONNECT_RETRY_MS = 50;
static const int CONNECT_TIMEOUT_MS = 4000;

std::mutex g_out_mutex;

int connect_to_server()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    sockaddr_in addr; std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = inet_addr(TEST_HOST);

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int res = connect(sock, (sockaddr*)&addr, sizeof(addr));
    if (res < 0)
    {
        if (errno != EINPROGRESS)
        {
            close(sock); return -1;
        }
        // Wait for connection completion
        fd_set wf; FD_ZERO(&wf); FD_SET(sock, &wf);
        struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
        res = select(sock+1, NULL, &wf, NULL, &tv);
        if (res <= 0) { close(sock); return -1; }
        int err = 0; socklen_t len = sizeof(err);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0)
        { close(sock); return -1; }
    }
    // restore blocking
    fcntl(sock, F_SETFL, flags);
    return sock;
}

std::string send_http_request(const std::string &raw)
{
    int sock = connect_to_server();
    if (sock < 0) return "__CONNECT_FAIL__";
    ssize_t sent = send(sock, raw.c_str(), raw.size(), 0);
    if (sent < 0) { close(sock); return "__SEND_FAIL__"; }
    std::string response;
    char buf[4096];
    // simple read loop
    while (true)
    {
        ssize_t r = recv(sock, buf, sizeof(buf), 0);
        if (r > 0) response.append(buf, r);
        else break;
    }
    close(sock);
    return response;
}

bool basic_http_test()
{
    std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    std::string resp = send_http_request(req);
    if (resp.find("__CONNECT_FAIL__") != std::string::npos || resp.find("__SEND_FAIL__") != std::string::npos)
        return false;
    bool ok = resp.find("HTTP/1.1 200") != std::string::npos || resp.find("HTTP/1.0 200") != std::string::npos;
    // look for a fragment typical of index.html
    if (ok)
        ok = resp.find("<html") != std::string::npos || resp.find("<HTML") != std::string::npos;
    return ok;
}

bool not_found_test()
{
    std::string req = "GET /this/path/does/not/exist HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    std::string resp = send_http_request(req);
    if (resp.find("__CONNECT_FAIL__") != std::string::npos || resp.find("__SEND_FAIL__") != std::string::npos)
        return false;
    bool has404 = resp.find("404") != std::string::npos;
    return has404;
}

bool invalid_method_test()
{
    std::string req = "TRACE / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    std::string resp = send_http_request(req);
    if (resp.find("__CONNECT_FAIL__") != std::string::npos || resp.find("__SEND_FAIL__") != std::string::npos)
        return false;
    // Accept either 400 or 405 depending on implementation
    bool bad = resp.find(" 400 ") != std::string::npos || resp.find(" 405 ") != std::string::npos;
    return bad;
}

struct MultiClientResult { int id; bool success; std::string status_line; };

void multi_client_worker(int id, MultiClientResult &out)
{
    std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    std::string resp = send_http_request(req);
    bool ok = resp.find("HTTP/1.1 200") != std::string::npos || resp.find("HTTP/1.0 200") != std::string::npos;
    std::string status;
    if (ok)
    {
        size_t pos = resp.find("\r\n");
        if (pos != std::string::npos) status = resp.substr(0, pos);
    }
    out.id = id; out.success = ok; out.status_line = status;
}

bool multi_client_test(int client_count, std::vector<MultiClientResult> &results)
{
    results.resize(client_count);
    std::vector<std::thread> threads;
    for (int i=0;i<client_count;++i)
        threads.push_back(std::thread(multi_client_worker, i, std::ref(results[i])));
    for (size_t i=0;i<threads.size();++i) threads[i].join();
    int success=0; for (size_t i=0;i<results.size();++i) if (results[i].success) ++success;
    return success == client_count;
}

struct StressStats { int total; int ok; int failed; double seconds; };

void stress_worker(int iterations, std::atomic<int> &okCount, std::atomic<int> &failCount)
{
    for (int i=0;i<iterations;++i)
    {
        std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        std::string resp = send_http_request(req);
        if (resp.find("HTTP/1.1 200") != std::string::npos || resp.find("HTTP/1.0 200") != std::string::npos)
            ++okCount;
        else
            ++failCount;
    }
}

StressStats stress_test(int threads_count, int iterations_per_thread)
{
    std::atomic<int> ok(0), fail(0);
    auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    for (int t=0;t<threads_count;++t)
        threads.push_back(std::thread(stress_worker, iterations_per_thread, std::ref(ok), std::ref(fail)));
    for (size_t i=0;i<threads.size();++i) threads[i].join();
    auto end = std::chrono::steady_clock::now();
    StressStats stats; stats.total = threads_count * iterations_per_thread; stats.ok = ok.load(); stats.failed = fail.load();
    stats.seconds = std::chrono::duration_cast<std::chrono::duration<double> >(end-start).count();
    return stats;
}

bool wait_for_server_ready()
{
    int elapsed = 0;
    while (elapsed < CONNECT_TIMEOUT_MS)
    {
        int s = connect_to_server();
        if (s >= 0)
        {
            close(s); return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(CONNECT_RETRY_MS));
        elapsed += CONNECT_RETRY_MS;
    }
    return false;
}

int main()
{
    std::cout << "[TEST] Waiting for server readiness..." << std::endl;
    if (!wait_for_server_ready())
    {
        std::cerr << "[TEST] Server did not become ready on port " << TEST_PORT << std::endl;
        return 2;
    }

    // 1. Basic HTTP
    bool basic = basic_http_test();
    std::cout << "[TEST] Basic HTTP GET: " << (basic ? "PASS" : "FAIL") << std::endl;

    // 1b. 404 handling
    bool nf = not_found_test();
    std::cout << "[TEST] 404 Not Found: " << (nf ? "PASS" : "FAIL") << std::endl;

    // 1c. Invalid method handling
    bool invalid = invalid_method_test();
    std::cout << "[TEST] Invalid method (TRACE): " << (invalid ? "PASS" : "FAIL") << std::endl;

    // 2. Multi-client
    std::vector<MultiClientResult> mcResults;
    bool multi = multi_client_test(10, mcResults);
    std::cout << "[TEST] Multi-client (10): " << (multi ? "PASS" : "FAIL") << std::endl;
    if (!multi)
    {
        int okCount=0; for (size_t i=0;i<mcResults.size();++i) if (mcResults[i].success) ++okCount;
        std::cout << "[TEST]   Successful clients: " << okCount << "/" << mcResults.size() << std::endl;
    }

    // 3. Stress test
    StressStats s = stress_test(8, 50); // 8 threads * 50 requests = 400 requests
    std::cout << "[TEST] Stress: total=" << s.total << " ok=" << s.ok << " failed=" << s.failed
              << " time=" << s.seconds << "s RPS=" << (s.total / (s.seconds>0? s.seconds:1)) << std::endl;

    bool overall = basic && nf && invalid && multi && (s.failed == 0);
    std::cout << "[TEST] Overall: " << (overall ? "PASS" : "FAIL") << std::endl;
    return overall ? 0 : 1;
}
