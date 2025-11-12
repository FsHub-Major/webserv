// Minimal JS to drive POST/GET tests using fetch()
async function putFile() {
  const path = document.getElementById('put-file-path').value || '/upload/my.bin';
  const input = document.getElementById('put-file-input');
  const out = document.getElementById('put-file-result');
  out.textContent = 'Sending...';
  if (!input.files || input.files.length === 0) {
    out.textContent = 'Choose a file first';
    return;
  }
  const file = input.files[0];
  try {
    const res = await fetch(path, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/octet-stream',
        'Content-Length': String(file.size)
      },
      body: file
    });
    const text = await res.text().catch(() => '');
    out.textContent = `Status: ${res.status} ${res.statusText}\n` + text;
  } catch (e) {
    out.textContent = 'Error: ' + e;
  }
}

async function putText() {
  const path = document.getElementById('put-text-path').value || '/upload/note.txt';
  const body = document.getElementById('put-text-body').value || '';
  const out = document.getElementById('put-text-result');
  out.textContent = 'Sending...';
  try {
    const enc = new TextEncoder();
    const b = enc.encode(body);
    const res = await fetch(path, {
      method: 'POST',
      headers: {
        'Content-Type': 'text/plain; charset=UTF-8',
        'Content-Length': String(b.byteLength)
      },
      body: b
    });
    const text = await res.text().catch(() => '');
    out.textContent = `Status: ${res.status} ${res.statusText}\n` + text;
  } catch (e) {
    out.textContent = 'Error: ' + e;
  }
}

async function getUrl() {
  const path = document.getElementById('get-path').value || '/';
  const out = document.getElementById('get-result');
  out.textContent = 'Fetching...';
  try {
    const res = await fetch(path, { method: 'GET' });
    // Try to read as text; if binary, show length only
    let body = '';
    const ct = res.headers.get('Content-Type') || '';
    if (/text|json|svg|xml/.test(ct)) body = await res.text();
    else {
      const ab = await res.arrayBuffer();
      body = `[binary ${ab.byteLength} bytes]`;
    }
    out.textContent = `Status: ${res.status} ${res.statusText}\nContent-Type: ${ct}\n\n` + body;
  } catch (e) {
    out.textContent = 'Error: ' + e;
  }
}

// Expose for inline buttons if needed
window.putFile = putFile;
window.putText = putText;
window.getUrl = getUrl;
