//THIS WILL HANDLE THE LAYOUT OF THE ENGINE
// CHANGED WITH AI: Ported the Windows engine Layout.cpp to the web version.
// The Windows version renders to SDL3 textures; this web version instead builds
// the same layout list and serializes it to JSON for the browser frontend.
// The MeasureNodes / PositionNodes / CSS-application logic is kept identical so
// the rendered output matches the Windows browser.
#include <string>
#include <vector>
#include "Layout.h"
#include "DOMTree.h"

// CHANGED WITH AI: globals that the main program reads after parsing to build JSON.
WebColor g_backgroundColor = { 245, 245, 245, 255 }; // default white-ish, gets changed
std::string g_pageTitle = "New Tab";
std::string g_currentURL = ""; // set by main before parsing, used to resolve image src


//looks through all our CSS rules to find the one that matches our tag in our tree
CSSRule* FindID(std::string input)
{
        for (int i = 0; i < globalCSS.size(); i++)
        {
                if (globalCSS[i].id == input)
                {
                        return &globalCSS[i];
                }
        }

        return nullptr;
}

//now we want to get proprites
std::string FindProperty(CSSRule* rule, std::string propertyName)
{
        for (int i = 0; i < rule->properties.size(); i++)
        {
                std::string current = rule->properties[i];

                size_t pos = current.find(propertyName);
                if (pos == 0)
                {
                        size_t colon = current.find(":");
                        if (colon != std::string::npos)
                        {
                                return current.substr(colon + 1);
                        }
                }
        }
        return "";
}


//see if a node has absolute in the css
bool IsAbsolute(Node* node, int& outX, int& outY)
{
        CSSRule* id = FindID(node->tagValue);
        if (id == nullptr)
        {
                return false;
        }

        std::string position = FindProperty(id, "position");
        if (position.empty())
        {
                return false;
        }

        while (!position.empty() && std::isspace((unsigned char)position.front()))
                position.erase(position.begin());

        if (position.find("absolute") != std::string::npos)
        {
                std::string leftStr = FindProperty(id, "left");
                std::string topStr = FindProperty(id, "top");

                while (!leftStr.empty() && std::isspace((unsigned char)leftStr.front()))
                        leftStr.erase(leftStr.begin());
                if (!leftStr.empty() && std::isdigit(leftStr[0])) {
                        outX = std::stoi(leftStr);
                }

                while (!topStr.empty() && std::isspace((unsigned char)topStr.front()))
                        topStr.erase(topStr.begin());
                if (!topStr.empty() && std::isdigit(topStr[0])) {
                        outY = std::stoi(topStr);
                }

                return true;
        }

        return false;
}

//see if a node has flex in the css
bool IsFlex(Node* node)
{
        CSSRule* id = FindID(node->tagValue);
        if (id == nullptr)
        {
                return false;
        }

        std::string display = FindProperty(id, "display");
        if (display.empty())
        {
                return false;
        }

        while (!display.empty() && std::isspace((unsigned char)display.front()))
                display.erase(display.begin());

        return display.find("flex") != std::string::npos;
}


//for bg and text colors!
struct RGB { int r, g, b;  };

RGB hexToRgb(unsigned int hexValue)
{
        RGB color;
        color.r = (hexValue >> 16) & 0xFF;
        color.g = (hexValue >> 8) & 0xFF;
        color.b = hexValue & 0xFF;
        return color;
}

RGB ParseHexColor(std::string hex)
{
        while (!hex.empty() && std::isspace((unsigned char)hex.front()))
                hex.erase(hex.begin());

        while (!hex.empty() && std::isspace((unsigned char)hex.back()))
                hex.pop_back();

        if (!hex.empty() && hex[0] == '#')
        {
                hex = hex.substr(1);
        }

        if (hex.size() == 3)
        {
                hex = std::string() + hex[0] + hex[0]
                        + hex[1] + hex[1]
                        + hex[2] + hex[2];
        }

        if (hex.empty() || hex.size() != 6)
        {
                RGB error = { 0,0,0, };
                return error;
        }

        try {
                unsigned int hexValue = std::stoul(hex, nullptr, 16);
                return hexToRgb(hexValue);
        }
        catch (...)
        {
                RGB error = { 0,0,0, };
                return error;
        }
}


std::vector<Layout> layoutList; //list to store the layout


//measures our node stuff here
void MeasureNodes(Node* node, int fontsize)
{
        if (node->measured) return;

        CSSRule* id = FindID(node->tagValue);

        if (id != nullptr)
        {
                std::string fs = FindProperty(id, "font-size");

                if (!fs.empty())
                {
                        if (std::isdigit(fs[0]))
                        {
                                fontsize = std::stoi(fs) * 2; //we * by 2, cause it would be super small
                        }
                }
        }
        else if (node->tag == NODETYPE::START)
        {
                if (node->tagValue == "h1")
                {
                        fontsize = 96;
                }
                else if (node->tagValue == "p") {
                        fontsize = 48;
                }
                else if (node->tagValue == "a") {
                        fontsize = 36;
                }
                else if (node->tagValue == "span") {
                        fontsize = 36;
                }
        }
        if (node->tag == NODETYPE::START && node->tagValue == "img" && !node->src.empty())
        {
                node->measuredWidth = 200;
                node->measuredHeight = 150;
                node->measured = true;
                return;
        }


        if (node->tag == NODETYPE::TEXT)
        {
                // CHANGED WITH AI: fixed width estimate from 0.45 to 0.55 to match
                // the font's actual average advance width (~0.55 em). The old 0.45
                // caused text items to overlap in the re-flow because the estimated
                // width was smaller than the actual rendered width.
                node->measuredWidth = (int)(node->tagValue.size() * (fontsize * 0.55));
                node->measuredHeight = fontsize + 4;
                node->measured = true;
                return;
        }


        int totalH = 0;
        int maxW = 0;

        for (Node* child : node->children)
        {
                MeasureNodes(child, fontsize);

                totalH += child->measuredHeight;
                maxW = std::max(maxW, child->measuredWidth);
        }

        node->measuredWidth = maxW;
        node->measuredHeight = totalH;

        node->measured = true;
}


// CHANGED WITH AI: ported PercentDecode + ResolveURL from the Windows GUI.cpp so
// the web engine can resolve relative image src URLs against the page URL.
std::string PercentDecode(const std::string& src)
{
        std::string out;
        for (size_t i = 0; i < src.size(); i++)
        {
                if (src[i] == '%' && i + 2 < src.size())
                {
                        std::string hex = src.substr(i + 1, 2);
                        char decoded = (char)std::stoul(hex, nullptr, 16);
                        out += decoded;
                        i += 2;
                }
                else if (src[i] == '+')
                {
                        out += ' ';
                }
                else
                {
                        out += src[i];
                }
        }
        return out;
}

std::string ResolveURL(std::string src, std::string currentUrl)
{
        std::string decoded = "";

        src = PercentDecode(src);

        size_t i = 0;
        while (i < src.size())
        {
                if (src.substr(i, 5) == "&amp;")
                {
                        decoded += "&";
                        i += 5;
                }
                else {
                        decoded += src[i++];
                }
        }
        src = decoded;

        if (src.find("http://") == 0 || src.find("https://") == 0) return src;

        std::string base = currentUrl;
        if (base.find("://") == std::string::npos)
        {
                base = "https://" + base;
        }

        std::string domain = "";
        size_t protoend = base.find("://");
        if (protoend != std::string::npos)
        {
                size_t domainEnd = base.find("/", protoend + 3);
                if (domainEnd != std::string::npos)
                {
                        domain = base.substr(0, domainEnd);
                }
                else {
                        domain = base;
                }
        }


        std::string direcotry = domain;
        size_t lastSlash = base.rfind("/");
        size_t protoSlash = base.find("://");

        if (lastSlash != std::string::npos && lastSlash > protoSlash + 3)
        {
                direcotry = base.substr(0, lastSlash);
        }


        if (!src.empty() && src[0] == '/')
        {
                return domain + src;
        }


        return direcotry + "/" + src;
}


//this is the poistion part
// CHANGED WITH AI: uses WebColor instead of SDL_Color, otherwise identical to Windows.
void PositionNodes(Node* node, int& currentXpos, int& currentYpos, int fontsize, WebColor textColor, WebColor bgColor, bool hasBg, std::string currentHref, bool inFlex = false)
{

        if (node->tag == NODETYPE::START)
        {

                if (!node->href.empty())
                {
                        currentHref = node->href;
                }

                CSSRule* id = FindID(node->tagValue);

                if (id != nullptr)
                {
                        std::string fs = FindProperty(id, "font-size");

                        if (!fs.empty())
                        {
                                while (!fs.empty() && std::isspace((unsigned char)fs.front())) {
                                        fs.erase(fs.begin());
                                }

                                if (std::isdigit(fs[0]))
                                {
                                        fontsize = std::stoi(fs) * 2;
                                }
                        }



                        std::string bg = FindProperty(id, "background-color");
                        if (bg.empty())
                        {
                                bg = FindProperty(id, "background");
                        }
                        if (!bg.empty())
                        {
                                if (bg.find("var(") != std::string::npos)
                                {
                                        // skip CSS var() backgrounds (can't resolve them)
                                }
                                else
                                {
                                        RGB parsed = ParseHexColor(bg);
                                        if (node->tagValue == "body")
                                        {
                                                g_backgroundColor = { (unsigned char)parsed.r, (unsigned char)parsed.g, (unsigned char)parsed.b, 255 };
                                        }
                                        else {
                                                bgColor = { (unsigned char)parsed.r, (unsigned char)parsed.g, (unsigned char)parsed.b, 255 };
                                                hasBg = true;
                                        }
                                }
                        }


                        std::string tc = FindProperty(id, "color");
                        if (!tc.empty())
                        {
                                RGB parsed = ParseHexColor(tc);
                                textColor = { (unsigned char)parsed.r, (unsigned char)parsed.g, (unsigned char)parsed.b, 255 };
                        }


                }
                else {
                        //give it that predefined
                        if (node->tagValue == "h1") {
                                fontsize = 96;
                        }
                        else if (node->tagValue == "h2") {
                                fontsize = 72;
                        }
                        else if (node->tagValue == "h3") {
                                fontsize = 60;
                        }
                        else if (node->tagValue == "h4" || node->tagValue == "p" || node->tagValue == "li" || node->tagValue == "blockquote") {

                                fontsize = 48;
                        }
                        else if (node->tagValue == "h5") {
                                fontsize = 36;
                        }
                        else if (node->tagValue == "h6") {
                                fontsize = 28;
                        }
                        else if (node->tagValue == "a" || node->tagValue == "span") {
                                fontsize = 36;
                        }
                        else if (node->tagValue == "big") {
                                fontsize = 64;
                        }
                        else if (node->tagValue == "small" || node->tagValue == "sub" || node->tagValue == "sup") {

                                fontsize = 24;
                        }
                        else if (node->tagValue == "code" || node->tagValue == "pre") {

                                fontsize = 36;
                        }
                        else {
                                fontsize = 24;
                        }
                }



                //we are gonna handle our flex stuff here.
                if (IsFlex(node))
                {
                        currentYpos += fontsize + 6;

                        int flexX = currentXpos;
                        int flexY = currentYpos;
                        int tallestChild = 0;

                        for (Node* child : node->children)
                        {
                                int childX = flexX;
                                int childY = flexY;

                                PositionNodes(child, childX, childY, fontsize, textColor, bgColor, hasBg, currentHref, true);

                                tallestChild = std::max(tallestChild, child->measuredHeight);

                                flexX += child->measuredWidth + 8;

                                if (flexX > 2600)
                                {
                                        flexX = currentXpos;
                                        flexY += tallestChild + 8;
                                        tallestChild = 0;
                                }

                        }

                        currentYpos = flexY + tallestChild + 8;

                        currentYpos += tallestChild + 20;
                        currentXpos = 20;

                        return;
                }

                //handle absolute pos
                int absX = 0;
                int absY = 0;

                if (IsAbsolute(node, absX, absY))
                {
                        int savedX = currentXpos;
                        int savedY = currentYpos;

                        currentXpos = absX;
                        currentYpos = absY;

                        for (Node* child : node->children)
                        {
                                PositionNodes(child, currentXpos, currentYpos, fontsize, textColor, bgColor, hasBg, currentHref, false);
                        }

                        currentXpos = savedX;
                        currentYpos = savedY;

                        return;
                }




                if (!inFlex)
                {

                        if (node->tagValue == "div" || node->tagValue == "p" ||
                                node->tagValue == "h1" || node->tagValue == "h2" ||
                                node->tagValue == "h3" || node->tagValue == "tr" ||
                                node->tagValue == "li" || node->tagValue == "br")
                        {
                                currentYpos += fontsize + 6;
                                currentXpos = 20;
                        }
                }
                // table cells
                if (node->tagValue == "td" || node->tagValue == "th")
                {
                        currentXpos = ((currentXpos / 200) + 1) * 200;
                }


                //check if its an image
                if (node->tagValue == "img" && !node->src.empty())
                {
                        Layout imgLayout;
                        imgLayout.node = node;
                        imgLayout.x = currentXpos;
                        imgLayout.y = currentYpos;
                        imgLayout.isImage = true;

                        // CHANGED WITH AI: resolve the image src against the page URL (was done
                        // in GUI.cpp PreRender on Windows; here we do it at layout time so the
                        // frontend gets a fully-qualified URL in the JSON).
                        imgLayout.imageSrc = ResolveURL(node->src, g_currentURL);

                        imgLayout.fontSize = 0;
                        imgLayout.textColor = { 0,0,0,255 };
                        imgLayout.href = currentHref;

                        imgLayout.width = node->measuredWidth;
                        imgLayout.hight = node->measuredHeight;

                        layoutList.push_back(imgLayout);

                        currentXpos += node->measuredWidth + 15;

                        if (currentXpos > 1200) {
                                currentXpos = 20;
                                currentYpos += node->measuredHeight + 15;
                        }


                }




        }


        if (node->tag == NODETYPE::TEXT)
        {
                Layout layouttree;


                layouttree.x = currentXpos;
                layouttree.y = currentYpos;

                if (node->tagValue == "tr")
                {
                        currentYpos += fontsize + 6;
                        currentXpos = 20;
                }

                layouttree.node = node;


                layouttree.textColor = textColor;

                if (!currentHref.empty())
                {
                        layouttree.textColor = { 0, 50, 255, 255 }; //blue
                }
                layouttree.fontSize = fontsize;

                layouttree.bgColor = bgColor;

                layouttree.hasBg = hasBg;


                layouttree.href = currentHref;


                layouttree.width = node->measuredWidth;
                layouttree.hight = node->measuredHeight;

                layoutList.push_back(layouttree);


                currentXpos += node->measuredWidth + 8;

                currentXpos += (int)(node->tagValue.size() * (fontsize / 2)) + 8;

                if (currentXpos > 1400)
                {
                        currentXpos = 20;
                        currentYpos += fontsize + 4;
                }




        }


        for (Node* child : node->children)
        {
                PositionNodes(child, currentXpos, currentYpos, fontsize, textColor, bgColor, hasBg, currentHref, false);
        }


}


// CHANGED WITH AI: JSON string escaper for the web port (Windows didn't need this
// because it rendered text straight to SDL; we serialize to JSON instead).
std::string JsonEscape(const std::string& s)
{
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s)
        {
                switch (c)
                {
                        case '"': out += "\\\""; break;
                        case '\\': out += "\\\\"; break;
                        case '\n': out += "\\n"; break;
                        case '\r': out += "\\r"; break;
                        case '\t': out += "\\t"; break;
                        default:
                                if ((unsigned char)c < 0x20)
                                {
                                        char buf[8];
                                        snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                                        out += buf;
                                }
                                else
                                {
                                        out += c;
                                }
                }
        }
        return out;
}

std::string ColorToHex(const WebColor& c)
{
        char buf[8];
        snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
        return std::string(buf);
}


// CHANGED WITH AI: Ported the PreRender re-flow from the Windows GUI.cpp.
// The Windows version re-flows all layout items in PreRender() using actual
// text surface dimensions from TTF_RenderText_Solid. This web port re-flows
// using the estimated dimensions from MeasureNodes, which produces the same
// sequential left-to-right, line-by-line positioning with proper 15px line
// gaps. Without this re-flow, items used the raw PositionNodes positions which
// had tight 2px vertical gaps (causing overlap) and erratic horizontal spacing.
void ReflowLayout()
{
        int xtrack = 20;
        int ytrack = 120;
        int lasty = -1;
        int maxLineHeight = 0;

        for (size_t i = 0; i < layoutList.size(); i++)
        {
                int origY = layoutList[i].y; // save original y for line-break detection

                // detect a new line: the original y changed from the previous item
                if (lasty != -1 && origY > lasty)
                {
                        xtrack = 20;
                        ytrack += (maxLineHeight + 15);
                        maxLineHeight = 0;
                }
                lasty = origY;

                // handle table column offsets (items with x > 20)
                if (layoutList[i].x > 20)
                {
                        int colX = (xtrack > layoutList[i].x) ? xtrack : layoutList[i].x;
                        layoutList[i].x = colX;
                        xtrack = colX;
                }
                else
                {
                        layoutList[i].x = xtrack;
                }

                layoutList[i].y = ytrack;

                // CHANGED WITH AI: recalculate width/height for text items using
                // the DISPLAY font size from PositionNodes (not the inherited
                // font size from MeasureNodes which is too small). The Windows
                // PreRender does this via TTF_RenderText_Solid; here we use the
                // same 0.55 em-per-char estimate as MeasureNodes but with the
                // correct display font size. This prevents line overlaps.
                if (!layoutList[i].isImage && layoutList[i].fontSize > 0 && layoutList[i].node)
                {
                        layoutList[i].width = (int)(layoutList[i].node->tagValue.size() * (layoutList[i].fontSize * 0.55));
                        layoutList[i].hight = layoutList[i].fontSize + 4;
                }

                int itemHeight = layoutList[i].hight;
                if (itemHeight > maxLineHeight)
                {
                        maxLineHeight = itemHeight;
                }

                xtrack += (layoutList[i].width + 12);
        }
}

int LayoutTree(Node* node)
{
        layoutList.clear();

        int currentY = 120; // same as Windows
        int currentX = 10;
        int startingfontsize = 14;


        MeasureNodes(node, startingfontsize);


        WebColor startingTextColor = { 0, 0, 0, 255 };

        WebColor startingBgColor = { 0, 0, 0, 0 };
        bool startingHasBg = false;

        PositionNodes(node, currentX, currentY, startingfontsize, startingTextColor, startingBgColor, startingHasBg, "");

        // CHANGED WITH AI: re-flow the layout to match the Windows PreRender behavior.
        // This fixes text overlap and spacing issues by repositioning items sequentially.
        ReflowLayout();

        std::cout << "Layout Compleate." << std::endl;

        // CHANGED WITH AI: serialize the layout to JSON (Windows called IMPORT() +
        // SDL rendering; the web version prints JSON for the frontend instead).
        std::string json = "{";
        json += "\"title\":\"" + JsonEscape(g_pageTitle) + "\",";
        json += "\"bgColor\":\"" + ColorToHex(g_backgroundColor) + "\",";
        json += "\"items\":[";

        bool first = true;
        for (const Layout& l : layoutList)
        {
                if (!first) json += ",";
                first = false;

                if (l.isImage)
                {
                        json += "{\"type\":\"image\",";
                        json += "\"src\":\"" + JsonEscape(l.imageSrc) + "\",";
                        json += "\"x\":" + std::to_string(l.x) + ",";
                        json += "\"y\":" + std::to_string(l.y) + ",";
                        json += "\"width\":" + std::to_string(l.width) + ",";
                        json += "\"height\":" + std::to_string(l.hight) + ",";
                        json += "\"href\":\"" + JsonEscape(l.href) + "\"}";
                }
                else
                {
                        // skip empty text
                        if (l.node == nullptr || l.node->tagValue.empty()) continue;

                        json += "{\"type\":\"text\",";
                        json += "\"text\":\"" + JsonEscape(l.node->tagValue) + "\",";
                        json += "\"x\":" + std::to_string(l.x) + ",";
                        json += "\"y\":" + std::to_string(l.y) + ",";
                        json += "\"width\":" + std::to_string(l.width) + ",";
                        json += "\"height\":" + std::to_string(l.hight) + ",";
                        json += "\"fontSize\":" + std::to_string(l.fontSize) + ",";
                        json += "\"color\":\"" + ColorToHex(l.textColor) + "\",";
                        json += "\"href\":\"" + JsonEscape(l.href) + "\"";
                        if (l.hasBg)
                        {
                                json += ",\"bgColor\":\"" + ColorToHex(l.bgColor) + "\"";
                        }
                        json += "}";
                }
        }

        json += "]}";

        // Print with a delimiter so the mini-service can split logs from payload.
        std::cout << "__BROWSE_JSON__" << std::endl;
        std::cout << json << std::endl;

        return 0;
}
