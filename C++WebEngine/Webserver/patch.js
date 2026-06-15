const fs = require('fs');
let html = fs.readFileSync('index.html', 'utf8');

html = html.replace(
    /output\.textContent \+= 'Connected to server\.\\n';/g,
    `output.insertAdjacentHTML('beforeend', 'Connected to server.<br>copyright k754a 2026<br>check out the github <a href="https://github.com/k754a/C-Plus-Plus-Web-Engine/graphs/traffic" style="color: inherit; text-decoration: underline;">https://github.com/k754a/C-Plus-Plus-Web-Engine/graphs/traffic</a>, and <a href="https://stardance.hackclub.com/projects/5003" style="color: inherit; text-decoration: underline;">https://stardance.hackclub.com/projects/5003</a><br><br>');`
);

html = html.replace(/output\.textContent \+= text;/g, `output.appendChild(document.createTextNode(text));`);
html = html.replace(/output\.textContent \+= input\.value \+ '\\n';/g, `output.appendChild(document.createTextNode(input.value + '\\n'));`);
html = html.replace(/output\.textContent \+= '\\nConnection closed\.\\n';/g, `output.appendChild(document.createTextNode('\\nConnection closed.\\n'));`);

fs.writeFileSync('index.html', html);
console.log('index.html patched!');
