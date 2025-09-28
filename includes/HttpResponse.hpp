#pragma once

#include "ext_libs.hpp"
#include "HttpRequest.hpp"
#include "Config.hpp" 

class HttpResponse
{
private:
    int statusCode;
    std::string reasonPhrase;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string fullResponse;

public:

    // Setters
    void setStatusCode(int code);
    void setReasonPhrase(const std::string& phrase);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void setContentType(const std::string& contentType);
    void setContentLength(size_t length);

    // Getters
    int getStatusCode() const;
    const std::string& getReasonPhrase() const;
    const std::string& getHeader(const std::string& key) const;
    const std::string& getBody() const;
    const std::map<std::string, std::string>& getHeaders() const;
    
    static std::string createResponse(const HttpRequest &request, const ServerConfig& config);

    // Helper method to get reason phrase from status code
    static std::string getReasonPhraseFromCode(int statusCode);
    
    // Method to automatically set Content-Length based on body
    void updateContentLength();
    
    // Optimization helpers to reduce code duplication
private:
    std::string normalizeUri(const std::string& uri) const;
    std::string getContentType(const std::string& path) const;
    std::string buildResponse(const HttpRequest &request, int statusCode, 
            const std::string& contentType = "", const std::string& body = "") const;
    void createOkResponse(const HttpRequest &request);
    std::string createErrorResponse(const HttpRequest &request, int errorCode) const;
    const std::string createGetResponse(const HttpRequest &request, const ServerConfig& config);
    const std::string createPostResponse(const HttpRequest &request,  const ServerConfig& config) const;
    const std::string createDeleteResponse(const HttpRequest &request,  const ServerConfig& config) const;
    const std::string createUnknowResponse(const HttpRequest &request,  const ServerConfig& config) const;
};