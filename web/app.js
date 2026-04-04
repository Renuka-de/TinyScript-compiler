const source = document.getElementById('source');
const output = document.getElementById('output');
const compile = document.getElementById('compile');
const download = document.getElementById('download');

compile.addEventListener('click', async () => {
  const code = source.value.trim();
  if (!code) {
    output.textContent = 'Insert TinyScript code first.';
    return;
  }
  output.textContent = 'Compiling...';
  try {
    const response = await fetch('/compile', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ code })
    });
    const text = await response.text();
    output.textContent = text;
  } catch (error) {
    output.textContent = 'Server error: ' + error.message;
  }
});

download.addEventListener('click', () => {
  const blob = new Blob([source.value], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'program.ts';
  a.click();
  URL.revokeObjectURL(url);
});
