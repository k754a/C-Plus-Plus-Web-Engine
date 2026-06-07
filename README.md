# Browser Engine Notes

FYI: I've definitely missed something writing this, so feel free to correct me; anything helps!

Just a note, this very barebones browser engine will be really, really simple, with only loading HTML and CSS, not running JavaScript, as well as not supporting security calls, and it's going to be very, VERY unoptimized.

A “browser engine" is like Google; however, I'm going to be coding a rendering engine. A rendering engine is a part of the browser engine that processes things like HTML and CSS to display on screen with some extra stuff for grabbing website HTML and CSS and to connect to the website. Think of CSS and HTML as music on Spotify. The music doesn't get played without the app; the music isn't the program, the Spotify app is. This is the same thing with a browser engine.

But how does a browser take a website and convert it into an accessible view for anyone to look at?

The first step to navigating to a website is finding where the assets for that website you are trying to view are. This is possible thanks to a DNS (Domain Name System) that works like a phonebook and translates domains like `google.com` into readable IP addresses (like `143.250.191.46`).

After we know what the IP is, the browser will connect to a server using a TCP handshake. This is designed so that 2 things (the browser and the server) can decide the parameters of the TCP before transiting data, usually over HTTPS. Mozilla has a very good description of how this handshake works:

> “Once the IP address is known, the browser sets up a connection to the server via a TCP three-way handshake. This mechanism is designed so that two entities attempting to communicate — in this case the browser and web server — can negotiate the parameters of the network TCP socket connection before transmitting data, often over HTTPS. TCP's three-way handshaking technique is often referred to as "SYN-SYN-ACK" — or more accurately SYN, SYN-ACK, ACK — because there are three messages transmitted by TCP to negotiate and start a TCP session between two computers. Yes, this means three more messages back and forth between each server, and the request has yet to be made.”

Source: https://developer.mozilla.org/en-US/docs/Web/Performance/Guides/How_browsers_work

However, for this implementation, I'm going to attempt to avoid coding this part for now, as this is a nightmare to code a security protocol to interface with current HTTPS without any libraries (I might code this later). We are going to attempt to communicate with HTTP, which is much less secure but will be much easier.

However, after TLS (another protocol), we finally start getting stuff from the server; an example would be an HTML sheet. The first connection we get 14KB of data, and as we keep receiving packets, we slowly increase this until we can determine the maximum network bandwidth. This prevents the server having to send lots of extra packets if the network hangs. This is called a TCP slow start algorithm.

After we receive the first packet of information, we can start parsing the information received. Parsing is the step where we take the data received and put it into the DOM and CSSOM, which is used by the renderer to “paint” the page on the screen.

Even if the requested page’s HTML is larger than the initial 14 KB, the browser will attempt to parse and render the data it has. This could be a potential issue if I don't build a way to continue pulling information for the HTML, as we can have inconsistent loading and fail to render on some websites.

Then, after pulling the data, we start to build a DOM (document object model). This DOM will be the most difficult thing to code.

Because HTML and CSS are in bytes right now (ones and zeros), the DOM first converts them to characters. It translates raw bytes to text characters based on the file coding (usually utf-8). Then we tokenize this from a bunch of text to distinct tokens, like this.

Then we will perform lexing (converting tokens to nodes), taking that HTML token I provided as an example and assigning it a node/class, creating nesting classes with distinct tokens throughout.

Finally, we link everything together into a tree.

We now need to build the CSSOM tree. The CSSOM tree is similar to the DOM tree; they both are independent data structures that allow viewing and rendering of a webpage, but this DOM basically tells the HTML where to be put as well as draws some elements.

The final step is loading JavaScript; for now JavaScript will be ignored, but as I work on it, I might add it in. However, I will NOT be coding this piece from scratch, as building my own JavaScript would be insane.

## Planned Steps

My steps go as follows:

1. The first plan is to code an HTTP loader that will download webpages.
2. Then an HTML parser. This parser will read the HTML, see tags like `<html>` and `<body>`, and build a DOM tree structure.
3. Then we parse the CSS, and for now to keep it simple it's going to parse super basic things, like tag selectors and maybe classes later.
4. Then we are going to go through the DOM and compare our CSS tags to see and attach styles to nodes.
5. Next we are going to build a layout system to convert the DOM tree into screen positions.
6. Finally, we are going to implement the rendering engine, which will allow us to render text and images on the screen from the website.

This is the first starting plan; however, once I get this basic idea working, we are going to move on to start attempting to implement things like more complex CSS and better HTML parsing that will let me open real websites (as the current idea described will be tested on an HTTP localhost with just basic HTML), and this will be refined to allow opening websites, maybe just text at first for any HTTP sites. I'll also implement some sort of dynamic inline layout using divs and spans.

## Closing Thoughts

Overall, this will be one of the most complex projects I've ever tried/attempted to take on; this one's up my alley at https://github.com/k754a/C-Custom-Code-Editor by a long shot.

This will 100% test my research, development, and C++ skills, but I believe that even if I'm not able to pull this off, I will learn a lot about how browsers work and good C++ practices.

I hope you will follow my progress on GitHub and Stardance as I build this!

Thanks for reading this! (Or if you just skipped to the end, that's all good lol.)

---

# Works Cited / Used as Research

* How modern browsers work, addyosmani, https://medium.com/@addyosmani/how-modern-browsers-work-7e1cc7337fff
* https://www.saperis.io/blog/how-does-a-web-browser-work
* https://jarvish.hashnode.dev/what-is-web-engine-and-its-working
* https://gizmodo.com/which-browser-engine-powers-your-web-browsing-and-why-d-1833935288
* https://medium.com/@andriyyevseytsev/web-engines-and-webkit-4c23456665fd
* https://www.swhosting.com/en/blog/what-is-a-web-browser-and-how-does-it-work
* https://stackoverflow.com/questions/51533097/how-browser-engine-works
* https://hacks.mozilla.org/2017/05/quantum-up-close-what-is-a-browser-engine
* https://bytebytego.com/guides/how-does-the-browser-render-a-web-page
* https://codefinity.com/blog/How-the-Web-Browser-Renders-a-Web-Page
* https://engineering.teknasyon.com/what-is-the-dom-how-does-html-rendering-happen-in-browsers-cbeb12bdfea6
* https://starkie.dev/blog/how-a-browser-renders-a-web-page
* https://blog.logrocket.com/how-browser-rendering-works-behind-scenes
* https://component-odyssey.com/tips/02-how-does-the-browser-render-html
* Browser engine. Wikipedia. https://en.wikipedia.org/wiki/Browser_engine
* How browser rendering works. GitHub Pages. https://dbaron.github.io/browser-rendering
* How browsers work | Articles. web.dev. https://web.dev/articles/howbrowserswork
* How does a Browser render a Webpage? DEV Community. https://dev.to/anuradha9712/how-does-a-browser-render-a-webpage-2en8
* How does html rendering works basically? The freeCodeCamp Forum. https://forum.freecodecamp.org/t/how-does-html-rendering-works-basically/155189
* Let's build a browser engine! Part 1: Getting started. https://limpet.net/mbrubeck/2014/08/08/toy-layout-engine-1.html
* Millar, Sandy, and Shreyas Naphad. *How Browser Rendering Works: A Step-by-Step Breakdown.* https://medium.com/@wutamy77/how-browser-rendering-works-a-step-by-step-breakdown-b74d5b9e0ee4
* Populating the page: how browsers work - Performance | MDN. https://developer.mozilla.org/en-US/docs/Web/Performance/Guides/How_browsers_work
* Web Browser Engine - Definition & Working. GeeksforGeeks. https://www.geeksforgeeks.org/techtips/web-browser-engine-definition-working
