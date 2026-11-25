#include "http_hal_windows_linux.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#if defined(__linux__) || defined(__APPLE__)
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace {

const char* const HTTP_SCHEME = "http://";
const char* const DEFAULT_PORT = "80";
const char* const HEADER_END = "\r\n\r\n";
const int READ_TIMEOUT_S = 3;

struct Url {
  std::string host;
  std::string port;
  std::string path;
};

bool parseUrl(const std::string& url, Url& parsed) {
  const size_t schemeLength = strlen(HTTP_SCHEME);
  if (url.compare(0, schemeLength, HTTP_SCHEME) != 0) {
    return false;
  }
  const size_t pathStart = url.find('/', schemeLength);
  const std::string authority = url.substr(schemeLength, pathStart - schemeLength);
  parsed.path = pathStart == std::string::npos ? "/" : url.substr(pathStart);
  const size_t colon = authority.find(':');
  parsed.host = authority.substr(0, colon);
  parsed.port = colon == std::string::npos ? DEFAULT_PORT : authority.substr(colon + 1);
  return !parsed.host.empty();
}

#if defined(__linux__) || defined(__APPLE__)
int connectTo(const Url& url) {
  addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* addresses = nullptr;
  if (getaddrinfo(url.host.c_str(), url.port.c_str(), &hints, &addresses) != 0) {
    return -1;
  }
  int sock = socket(addresses->ai_family, addresses->ai_socktype, addresses->ai_protocol);
  if (sock >= 0 && connect(sock, addresses->ai_addr, addresses->ai_addrlen) != 0) {
    close(sock);
    sock = -1;
  }
  freeaddrinfo(addresses);
  if (sock >= 0) {
    timeval timeout = {READ_TIMEOUT_S, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  }
  return sock;
}

bool headersSayOk(const std::string& headers, size_t expectedLength) {
  const bool statusOk = headers.find(" 200 ") != std::string::npos;
  const std::string lengthHeader = "Content-Length: " + std::to_string(expectedLength) + "\r\n";
  return statusOk && headers.find(lengthHeader) != std::string::npos;
}

bool runGet(const std::string& rawUrl, uint8_t* body, size_t expectedLength) {
  Url url;
  if (!parseUrl(rawUrl, url)) {
    return false;
  }
  const int sock = connectTo(url);
  if (sock < 0) {
    return false;
  }
  const std::string request = "GET " + url.path + " HTTP/1.1\r\nHost: " + url.host +
                              "\r\nConnection: close\r\n\r\n";
  send(sock, request.data(), request.size(), 0);

  std::string received;
  char chunk[4096];
  ssize_t got;
  while ((got = recv(sock, chunk, sizeof(chunk), 0)) > 0) {
    received.append(chunk, (size_t)got);
  }
  close(sock);

  const size_t headerEnd = received.find(HEADER_END);
  if (headerEnd == std::string::npos) {
    return false;
  }
  const size_t bodyStart = headerEnd + strlen(HEADER_END);
  if (!headersSayOk(received.substr(0, bodyStart), expectedLength) ||
      received.size() - bodyStart != expectedLength) {
    return false;
  }
  memcpy(body, received.data() + bodyStart, expectedLength);
  return true;
}
#else
// The Windows simulator has no art: it fails every GET, so the cover stays on its placeholder.
bool runGet(const std::string&, uint8_t*, size_t) {
  return false;
}
#endif

}  // namespace

bool start_http_get_HAL(const char* url, uint8_t* body, size_t expectedLength, HttpGetHandover* handover) {
  if (!handover->begin()) {
    return false;
  }
  const std::string urlCopy = url;
  std::thread([urlCopy, body, expectedLength, handover]() {
    handover->finish(runGet(urlCopy, body, expectedLength));
  }).detach();
  return true;
}

uint8_t* allocate_large_buffer_HAL(size_t bytes) {
  return (uint8_t*)malloc(bytes);
}

void free_large_buffer_HAL(uint8_t* buffer) {
  free(buffer);
}
