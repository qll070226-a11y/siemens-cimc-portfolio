from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "APP" / "MDK" / "CIMC_APP.uvprojx"


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


tree = ET.parse(PROJECT)
paths = [node.text for node in tree.findall(".//FilePath") if node.text]
resolved = [(PROJECT.parent / path.replace("\\", "/")).resolve() for path in paths]
if any(not path.exists() for path in resolved):
    fail("project references missing files")
if paths.count(r"..\User\contest_main.c") != 1:
    fail("contest main is not unique")
if paths.count(r"..\User\contest_irq_all.c") != 1:
    fail("contest IRQ is not unique")
if paths.count(r"..\ThirdParty\FreeModbus\port\contest_port_runtime.c") != 1:
    fail("fixed FreeModbus port is not selected")

port = (ROOT / "APP/ThirdParty/FreeModbus/port/contest_port_runtime.c").read_text()
if "USART_FLAG_TC" not in port or "RS485_CS_SET(0)" not in port:
    fail("RS485 receive transition lacks TC wait or DE release")

config = (ROOT / "APP/HeaderFiles/contest_config.h").read_text()
for macro in ("CONTEST_SAMPLE_PERIOD_MS", "CONTEST_LOG_PERIOD_MS", "CONTEST_WIRE_BREAK_MA"):
    if not re.search(rf"#define\s+{macro}\s+", config):
        fail(f"missing configuration macro: {macro}")

modbus = (ROOT / "APP/Function/contest_modbus.c").read_text()
for callback in ("eMBRegInputCB", "eMBRegHoldingCB", "eMBRegCoilsCB", "eMBRegDiscreteCB"):
    if callback not in modbus:
        fail(f"missing Modbus callback: {callback}")

print("[OK] project references and source invariants passed")
sys.exit(0)
