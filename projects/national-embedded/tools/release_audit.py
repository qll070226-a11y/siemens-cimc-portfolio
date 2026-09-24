from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = root / "APP/MDK/CIMC_APP.uvprojx"
paths = [n.text for n in ET.parse(project).findall(".//FilePath") if n.text]
assert len(paths) == len(set(paths))
assert paths.count(r"..\User\contest_main.c") == 1
assert paths.count(r"..\User\contest_irq_all.c") == 1
assert paths.count(r"..\ThirdParty\FreeModbus\port\contest_port_clocked.c") == 1
assert not [p for p in paths if not (project.parent / p.replace("\\", "/")).resolve().exists()]
port = (root / "APP/ThirdParty/FreeModbus/port/contest_port_clocked.c").read_text(encoding="utf-8")
for token in ("static volatile eMBEventType", "USART_FLAG_TC", "RS485_CS_SET(0)",
              "RCU_TIMER_PSC_MUL2", "SystemCoreClock / 2U", "1000000U"):
    assert token in port, token
print("[OK] release project audit passed")
