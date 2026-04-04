import os
import subprocess

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
COMPILER = os.path.join(ROOT, 'tinyscript.exe' if os.name == 'nt' else 'tinyscript')
SAMPLES = [
    os.path.join(ROOT, 'examples', 'thermostat.ts'),
    os.path.join(ROOT, 'examples', 'event_control.ts'),
    os.path.join(ROOT, 'examples', 'schedule.ts')
]

if not os.path.exists(COMPILER):
    print('Compiler binary not found. Build first.')
    raise SystemExit(1)

for sample in SAMPLES:
    print(f'=== Running {os.path.basename(sample)} ===')
    result = subprocess.run([COMPILER, sample, '--run', '--emit-ir'], capture_output=True, text=True)
    print(result.stdout)
    if result.returncode != 0:
        print('Error:', result.stderr)
        raise SystemExit(result.returncode)
print('All sample tests passed.')
