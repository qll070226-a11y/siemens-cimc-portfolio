from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = root / "APP/MDK/CIMC_APP.uvprojx"
paths = [n.text for n in ET.parse(project).findall(".//FilePath") if n.text]
assert len(paths) == len(set(paths))
for expected in (
    r"..\User\contest_main.c",
    r"..\User\contest_irq_all.c",
    r"..\ThirdParty\FreeModbus\port\contest_port_clocked.c",
    r"..\Function\contest_storage_final.c",
):
    assert paths.count(expected) == 1, expected
assert not [p for p in paths if not (project.parent / p.replace("\\", "/")).resolve().exists()]
port = (root / "APP/ThirdParty/FreeModbus/port/contest_port_clocked.c").read_text(encoding="utf-8")
for token in ("static volatile eMBEventType", "USART_FLAG_TC", "RS485_CS_SET(0)",
              "RCU_TIMER_PSC_MUL2", "SystemCoreClock / 2U", "1000000U"):
    assert token in port, token
storage = (root / "APP/Function/contest_storage_final.c").read_text(encoding="utf-8")
assert "%f" not in storage and "%.3f" not in storage
assert "if (result != FR_OK) return -3;" in storage
print("[OK] release project audit v2 passed")
