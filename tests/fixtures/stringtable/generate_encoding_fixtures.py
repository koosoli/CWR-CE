from pathlib import Path
import csv
import io

ROOT = Path(__file__).resolve().parent
rows = [
    ["LANGUAGE", "English", "French", "German", "Spanish", "Czech"],
    ["STR_COPYRIGHT", "© 2026 Fixture Lab", "© 2026 Fixture Lab", "© 2026 Fixture Lab", "© 2026 Fixture Lab", "© 2026 Fixture Lab"],
    ["STR_ACCENT", "cafe naive", "café naïve", "Käse Straße", "señor", "prilis zlutoucky"],
    ["STR_GREETING", "Hello", "Bonjour", "Grüß Gott", "Hola", "Ahoj"],
    ["STR_EURO", "100€", "100€", "100€", "100€", "100€"],
    ["STR_ASCII", "plain", "plain", "plain", "plain", "plain"],
]
buf = io.StringIO(newline="")
csv.writer(buf, lineterminator="\r\n").writerows(rows)
ROOT.joinpath("stringtable_cp1252.csv").write_bytes(buf.getvalue().encode("cp1252"))
ROOT.joinpath("stringtable_ru_cp1251.csv").write_bytes(
    "LANGUAGE,English,Russian\r\n".encode("ascii")
    + "STR_SUNDAY,Sunday,Воскресенье\r\n".encode("cp1251")
    + "STR_MONDAY,Monday,Понедельник\r\n".encode("cp1251")
    + "STR_COPYRIGHT,Copyright,Синтетическая таблица\r\n".encode("cp1251")
)

credits_rows = [
    ["LANGUAGE", "English", "French", "Italian", "Spanish", "German", "Czech", "Russian"],
    ["STR_CREDITSN01", *["Krad Štěpěl"] * 7],
    ["STR_CREDITSN07", *["Flar Řekán\\nBuboš Pěcal"] * 7],
    ["STR_DN_CREDITSN07", *["Qend Řatěj\\nVorbi Kovác\\nFlowshot"] * 7],
]
buf = io.StringIO(newline="")
csv.writer(buf, lineterminator="\r\n").writerows(credits_rows)
ROOT.joinpath("stringtable_credits_cp1250.csv").write_bytes(buf.getvalue().encode("cp1250"))

base_rows = [
    ["LANGUAGE", "English"],
    ["STR_PACKAGE_BASE_ONLY", "Base package"],
]
buf = io.StringIO(newline="")
csv.writer(buf, lineterminator="\r\n").writerows(base_rows)
ROOT.joinpath("stringtable_package.csv").write_bytes(buf.getvalue().encode("cp1252"))

shard_rows = [
    ["LANGUAGE", "English"],
    ["STR_DISP_MAIN_OPT_AUDIO", "Audio"],
    ["STR_DISP_MAIN_OPT_GAME", "Game"],
    ["STR_DISP_MAIN_OPT_AUDIO_OUTPUT", "Output device"],
]
buf = io.StringIO(newline="")
csv.writer(buf, lineterminator="\r\n").writerows(shard_rows)
ROOT.joinpath("stringtable_package_mainmenu.utf8.csv").write_bytes(buf.getvalue().encode("utf-8"))
