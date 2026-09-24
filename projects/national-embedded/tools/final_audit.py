from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = root / "APP/MDK/CIMC_APP.uvprojx"
paths = [n.text for n in ET.parse(project).findall(".//FilePath") if n.text]
assert paths.count(r"..\User\contest_main.c") == 1
assert paths.count(r"..\User\contest_irq_all.c") == 1
assert paths.count(r"..\ThirdParty\FreeModbus\port\contest_port_final.c") == 1
assert not [p for p in paths if not (project.parent / p.replace("\\", "/")).resolve().exists()]
port = (root / "APP/ThirdParty/FreeModbus/port/contest_port_final.c").read_text(encoding="utf-8")
assert "static volatile eMBEventType" in port
assert "static volatile BOOL" in port
assert "USART_FLAG_TC" in port and "RS485_CS_SET(0)" in port
print("[OK] final project audit passed")
