#pragma once

#include "ext_libs.hpp"
#include "HttpRequest.hpp"

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
    
    static std::string createResponse(const HttpRequest &request);

    // Helper method to get reason phrase from status code
    static std::string getReasonPhraseFromCode(int statusCode);
    
    // Method to automatically set Content-Length based on body
    void updateContentLength();
    private:
        void createOkResponse(const HttpRequest &request);
        std::string createErrorResponse(const HttpRequest &request, int errorCode) const;
        const std::string createGetResponse(const HttpRequest &request);
        const std::string createPostResponse(const HttpRequest &request) const;
        const std::string createDeleteResponse(const HttpRequest &request) const;
        const std::string createUnknowResponse(const HttpRequest &request) const;
};