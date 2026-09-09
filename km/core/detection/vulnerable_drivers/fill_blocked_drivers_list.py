import requests, re

r = requests.get("https://www.loldrivers.io/api/drivers.json")
drivers = r.json()

def hex_to_c_array(hex_str):
    if len(hex_str) % 2 != 0:
        return None
    return ", ".join(f"0x{hex_str[i:i+2]}" for i in range(0, len(hex_str), 2))

entries = {}
for d in drivers:
    for sample in d.get("KnownVulnerableSamples", []):

        sha256 = sample.get("SHA256", "")
        if isinstance(sha256, str):
            sha256 = sha256.strip().upper()
        else:
            continue

        raw_ah = sample.get("Authentihash", "")
        if isinstance(raw_ah, dict):
            authentihash = raw_ah.get("$", raw_ah.get("SHA256", "")).strip().upper()
        elif isinstance(raw_ah, str):
            authentihash = raw_ah.strip().upper()
        else:
            authentihash = ""

        filename = sample.get("Filename", "")
        if isinstance(filename, str):
            filename = filename.strip().replace('"', '').replace('\\', '\\\\')
        else:
            filename = ""

        if len(sha256) != 64:
            continue

        sha256_arr       = hex_to_c_array(sha256)
        authentihash_arr = hex_to_c_array(authentihash) if len(authentihash) == 64 else None

        if not sha256_arr:
            continue

        entries[sha256] = (sha256_arr, authentihash_arr, filename)

lines = []
lines.append("static const VULNERABLE_DRIVER_ENTRY VulnerableDriverHashes[] = {\n")
for sha256, (sha256_arr, authentihash_arr, filename) in sorted(entries.items()):
    ah = authentihash_arr if authentihash_arr else "0x00" + ", 0x00" * 31
    lines.append(f'    {{ {{ {sha256_arr} }}, {{ {ah} }}, "{filename}" }},\n')
lines.append("};\n")
lines.append(f"static const ULONG VulnerableDriverCount = {len(entries)};\n")
new_content = "".join(lines).strip()

with open("blocked_drivers.hpp", "r") as f:
    original = f.read()

pattern = r"static const VULNERABLE_DRIVER_ENTRY VulnerableDriverHashes\[\].*?static const ULONG VulnerableDriverCount = \d+;"
updated = re.sub(pattern, new_content, original, flags=re.DOTALL)

with open("blocked_drivers.hpp", "w") as f:
    f.write(updated)

print(f"Actualizado: {len(entries)} entradas")