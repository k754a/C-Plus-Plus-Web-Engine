#include "ConnectSocket.h"
#include "Parser.h"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

//=======CLEANUP=======\\
// (SplitURL stays exactly as you have it, or you can skip it entirely —
// curl can take the full URL directly, see note below)

//=======CURL WRITE CALLBACKS=======\\

// Used for text/html responses
static size_t WriteStringCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userdata);
    response->append(ptr, totalSize);
    return totalSize;
}

// Used for binary (image) responses
static size_t WriteVectorCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t totalSize = size * nmemb;
    std::vector<unsigned char>* buffer = static_cast<std::vector<unsigned char>*>(userdata);
    buffer->insert(buffer->end(), ptr, ptr + totalSize);
    return totalSize;
}

//=======DOWNLOAD IMAGES=======\\

std::vector<unsigned char> DownloadImages(std::string url, bool usingLocal)
{
    std::vector<unsigned char> result;

    if (usingLocal)
    {
        std::ifstream file(url, std::ios::binary);
        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to init curl" << "\n";
        return result;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteVectorCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);        // follow redirects
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: image/webp,image/png,image/jpeg,image/*,*/*");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        // silent fail on purpose, same as your original comment noted
        result.clear();
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
} //END OF DownloadImages

//=======DOWNLOAD HTTPS=======\\

int ConnectSocketHTTPS(std::wstring input)
{
    // convert wstring -> string for curl (curl doesn't take wide strings)
    std::string url(input.begin(), input.end());

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to init curl" << "\n";
        return -1;
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers,
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
    headers = curl_slist_append(headers, "Upgrade-Insecure-Requests: 1");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        std::string finalERROR = "<h1> AW SNAP - Something went wrong...</h1><h3> Reason: "
            + std::string(curl_easy_strerror(res)) + "</h3>";
        Parser(finalERROR);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return -1;
    }

    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (statusCode >= 400)
    {
        std::string finalERROR;
        if (statusCode == 404)
            finalERROR = "<h1> [NETWORK ERROR]</h1><h3> Request failed with status 404 - Page not found. </h3>";
        else
            finalERROR = "<h1> [NETWORK ERROR]</h1><h3> Request failed with status "
                + std::to_string(statusCode) + "</h3>";

        Parser(finalERROR);
        return -1;
    }

    Parser(response);
    return 0;
} //END OF ConnectSocketHTTPS
