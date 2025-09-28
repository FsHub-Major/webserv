#pragma once

#include "ext_libs.hpp"
#include "HttpRequest.hpp"
#include "Config.hpp" 

/**
 * @class HttpResponse
 * @brief Builds HTTP responses for GET/POST/DELETE and utilities.
 *
 * Responsibilities:
 * - Parse and sanitize target paths
 * - Determine content type
 * - Compose status line, headers, and body
 * - Create method-specific responses and error pages
 */
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
    /** Set numeric status code and derive reason phrase. */
    void setStatusCode(int code);
    /** Override reason phrase explicitly. */
    void setReasonPhrase(const std::string& phrase);
    /** Set or replace a header key/value. */
    void setHeader(const std::string& key, const std::string& value);
    /** Set body and auto-update Content-Length. */
    void setBody(const std::string& body);
    /** Convenience to set Content-Type. */
    void setContentType(const std::string& contentType);
    /** Explicitly set Content-Length header. */
    void setContentLength(size_t length);

    // Getters
    /** Return numeric status code. */
    int getStatusCode() const;
    /** Return textual reason phrase. */
    const std::string& getReasonPhrase() const;
    /** Return header value by key or empty string if not found. */
    const std::string& getHeader(const std::string& key) const;
    /** Return response body. */
    const std::string& getBody() const;
    /** Return all headers map. */
    const std::map<std::string, std::string>& getHeaders() const;
    
    /** Factory: dispatch by method to create a full HTTP response string. */
    static std::string createResponse(const HttpRequest &request, const ServerConfig& config);

    // Helper method to get reason phrase from status code
    /** Translate status code to reason phrase (e.g., 404 -> "Not Found"). */
    static std::string getReasonPhraseFromCode(int statusCode);
    
    // Method to automatically set Content-Length based on body
    /** Update Content-Length based on current body size. */
    void updateContentLength();
    
    // Optimization helpers to reduce code duplication
private:
    /** Sanitize URI (strip query, prevent traversal, remove leading '/'). */
    std::string normalizeUri(const std::string& uri) const;
    /** Guess MIME type from file extension. */
    std::string getContentType(const std::string& path) const;
    /** Compose a full HTTP response (status line, headers, and optional body). */
    std::string buildResponse(const HttpRequest &request, int statusCode, 
            const std::string& contentType = "", const std::string& body = "") const;
    /** Build a 200/204 response using current headers/body. */
    void createOkResponse(const HttpRequest &request);
    /** Build an HTML error page for given error code. */
    std::string createErrorResponse(const HttpRequest &request, int errorCode) const;
    /** Handle GET: serve static files under server root (with index fallback). */
    const std::string createGetResponse(const HttpRequest &request, const ServerConfig& config);
    /** Handle POST: currently returns 501 Not Implemented. */
    const std::string createPostResponse(const HttpRequest &request,  const ServerConfig& config) const;
    /** Handle DELETE: remove files (safeguarded). */
    const std::string createDeleteResponse(const HttpRequest &request,  const ServerConfig& config) const;
    /** Handle unknown/unsupported methods with 405 and Allow header. */
    const std::string createUnknowResponse(const HttpRequest &request,  const ServerConfig& config) const;
};