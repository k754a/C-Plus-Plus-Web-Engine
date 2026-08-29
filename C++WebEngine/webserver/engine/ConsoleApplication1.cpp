// ConsoleApplication1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// CHANGED WITH AI: Web port of the Windows Webengine.cpp main.
// The Windows version runs an SDL3 GUI loop. This web version is a one-shot CLI:
// it takes a URL (or "home") as argv, fetches + parses it, prints a JSON layout,
// and exits. This lets the Node.js mini-service spawn one short-lived process per
// navigation instead of keeping a process per user (important for a small shared
// server like Hat Club Nest).
#include <regex>
#include <iostream>
#include "ConnectSocket.h" //pulls our validate string global libary
#include "Parser.h"
#include "Layout.h"
#include <fstream>
#include <sstream>

//this checks if a url is valid or not.
const std::regex httpPattern("((http)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");
const std::regex httpsPattern("((https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)");

// CHANGED WITH AI: globals declared in Layout.cpp / Parser.cpp that we set before parsing.
extern WebColor g_backgroundColor;
extern std::string g_pageTitle;
extern std::string g_currentURL;

// CHANGED WITH AI: load a local HTML file (the homepage) and parse it, mirroring
// what the Windows GUI does for the Home button / new tab.
int LoadLocalHTML(const std::string& path)
{
        std::ifstream file(path);
        if (!file.is_open())
        {
                std::cout << "Could not open local file: " << path << std::endl;
                return -1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileinfo = buffer.str();
        Parser(fileinfo);
        return 0;
}

// CHANGED WITH AI: build the homepage HTML (same content the Windows UpdateHTML()
// produces) so a fresh "home" load shows the starred pages.
std::string BuildHomeHTML(const std::string& starredPath)
{
        std::string html;
        html += "<!DOCTYPE html><html lang=\"en\"><head>";
        html += "<link rel=\"icon\" href=\"data:,\">";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        html += "<title>New Tab</title>";
        html += "<style>";
        html += "h1 { color: #000000; font-size: 36px; }";
        html += "p { color: #111111; font-size: 24px; }";
        html += "a { color: #4A90E2; font-size: 22px; }";
        html += ".browser-image { max-width: 200px; height: auto; display: block; margin-top: 20px; margin-bottom: 20px; border-radius: 8px; }";
        html += "</style></head><body><div>";
        html += "<h1>C++Browse</h1>";
        html += "<p>This is my C++ web browser project.</p>";
        html += "<h1>To start, search anything.</h1>";
        // CHANGED WITH AI: homepage hint text. Bing will occasionally serve a
        // captcha page despite our real-Chrome UA — when that happens, the
        // workaround is to enter the full URL directly (e.g. https://example.com/)
        // instead of searching for it.
        html += "<p>Sometimes you will be captcha'd.</p>";
        html += "<p>Use full URLs (https://...) if that happens.</p>";

        // load starred pages
        std::vector<std::string> starred;
        std::ifstream sf(starredPath);
        if (sf.is_open())
        {
                std::string line;
                while (std::getline(sf, line))
                {
                        // CHANGED WITH AI: strip trailing \r (the STAR file uses CRLF).
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        if (!line.empty()) starred.push_back(line);
                }
        }

        if (!starred.empty())
        {
                html += "<br>Starred Pages:";
                for (const std::string& site : starred)
                {
                        html += "<p>-<a href=\"" + site + "\">" + site + "</a></p>";
                }
        }
        else
        {
                html += "<br>No Starred Pages";
        }

        html += "</div></body></html>";
        return html;
}

int main(int argc, char* argv[])
{
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(NULL);

        StartWinSock();

        // CHANGED WITH AI: take the target from argv instead of an infinite stdin loop.
        std::string input = (argc > 1) ? argv[1] : "home";

        // reset globals per run (one process = one page)
        g_backgroundColor = { 245, 245, 245, 255 };
        g_pageTitle = "New Tab";
        g_currentURL = "";

        if (input == "home" || input.empty())
        {
                // CHANGED WITH AI: parse the generated homepage, like the Windows Home button.
                std::string homeHtml = BuildHomeHTML("starred_pages.STAR");
                g_pageTitle = "New Tab";
                Parser(homeHtml);
        }
        else if (std::regex_match(input, httpPattern))
        {
                std::cout << "http url!" << std::endl;
                g_currentURL = input;
                ConnectSocketHTTP(input);
        }
        else if (std::regex_match(input, httpsPattern))
        {
                std::cout << "https url!" << std::endl;
                g_currentURL = input;
                ConnectSocketHTTPS(input);
        }
        else
        {
                // CHANGED WITH AI: treat non-URL input as a web search.
                //
                // ORIGINAL BEHAVIOR: searched DuckDuckGo's lite endpoint
                // (https://lite.duckduckgo.com/lite/?q=...). That endpoint is
                // unreachable from many shared servers (TCP connect hangs) and
                // also returns captchas aggressively to non-browser clients, so
                // searches silently failed — the frontend sat on "Loading..."
                // forever while the engine waited for a curl response that
                // never came.
                //
                // FIX: switched the default search backend to Bing, which
                // returns 200 OK with clean parseable HTML to a real-Chrome
                // User-Agent. Also added proper URL-encoding for the query so
                // characters like &, #, ?, +, % don't break the URL (the old
                // space-only hack would turn "AT&T" into "?q=AT&T" which Bing
                // would parse as two separate params).

                std::string query = "";
                for (unsigned char c : input)
                {
                        if ((c >= 'a' && c <= 'z') ||
                            (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') ||
                            c == '-' || c == '_' || c == '.' || c == '~')
                        {
                                query += (char)c;
                        }
                        else if (c == ' ')
                        {
                                query += '+';
                        }
                        else
                        {
                                // percent-encode anything else
                                char buf[4];
                                snprintf(buf, sizeof(buf), "%%%02X", c);
                                query += buf;
                        }
                }

                input = "https://www.bing.com/search?q=" + query;
                g_currentURL = input;
                std::cout << "search url: " << input << std::endl;
                ConnectSocketHTTPS(input);
        }

        EndWinSock();
        return 0;
}
