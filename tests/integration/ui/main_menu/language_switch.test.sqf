// Switch through all 7 supported languages and assert the top-level main
// menu captions follow the active language immediately.
//
// Covers both:
// - resource-backed root buttons (Campaign, Single, Multiplayer, Editor,
//   Options, Quit)
// - the code-driven Player caption (DisplayMain::OnDraw)
//
// triAssertText / triAssertControlText inspect control text, so this checks
// the language-refresh path even for glyphs the current font cannot render.
//
// Strings sourced from packages/Remaster/BIN/STRINGTABLE_MAINMENU.utf8.csv
// (the modern menu-shard table that wins over the legacy STRINGTABLE.CSV
// for keys defined in both).  When any caption changes there, this test
// must be updated in lockstep.

triSetLanguage "English"

triAssertEq [(triDisplay), 0]
triAssertIncludes [(triVisibleTexts), "CAMPAIGN GAME"]
triAssertIncludes [(triVisibleTexts), "OPTIONS"]
triAssertIncludes [(triVisibleTexts), "SINGLE MISSION"]
triAssertIncludes [(triVisibleTexts), "MISSION EDITOR"]
triAssertIncludes [(triVisibleTexts), "MULTIPLAYER"]
triAssertIncludes [(triVisibleTexts), "QUIT GAME"]

triSetLanguage "Czech"
triAssertIncludes [(triVisibleTexts), "KAMPAŇ"]
triAssertIncludes [(triVisibleTexts), "NASTAVENÍ"]
triAssertIncludes [(triVisibleTexts), "JEDNOTLIVÉ MISE"]
triAssertIncludes [(triVisibleTexts), "EDITOR MISÍ"]
triAssertIncludes [(triVisibleTexts), "SÍŤOVÁ HRA"]
triAssertIncludes [(triVisibleTexts), "KONEC HRY"]

triSetLanguage "French"
triAssertIncludes [(triVisibleTexts), "CAMPAGNE"]
triAssertIncludes [(triVisibleTexts), "OPTIONS"]
triAssertIncludes [(triVisibleTexts), "MISSION UNIQUE"]
triAssertIncludes [(triVisibleTexts), "EDITEUR DE MISSION"]
triAssertIncludes [(triVisibleTexts), "MULTIJOUEURS"]
triAssertIncludes [(triVisibleTexts), "QUITTER LE JEU"]

triSetLanguage "German"
triAssertIncludes [(triVisibleTexts), "KAMPAGNE  SPIELEN"]
triAssertIncludes [(triVisibleTexts), "OPTIONEN"]
triAssertIncludes [(triVisibleTexts), "EINZELEINSATZ"]
triAssertIncludes [(triVisibleTexts), "EINSATZEDITOR"]
triAssertIncludes [(triVisibleTexts), "MEHRSPIELER"]
triAssertIncludes [(triVisibleTexts), "SPIEL BEENDEN"]

triSetLanguage "Italian"
triAssertIncludes [(triVisibleTexts), "CAMPAGNA MILITARE"]
triAssertIncludes [(triVisibleTexts), "OPZIONI"]
triAssertIncludes [(triVisibleTexts), "MISSIONI SINGOLE"]
triAssertIncludes [(triVisibleTexts), "EDITOR MISSIONI"]
triAssertIncludes [(triVisibleTexts), "MULTIGIOCATORE"]
triAssertIncludes [(triVisibleTexts), "ESCI DAL GIOCO"]

triSetLanguage "Spanish"
triAssertIncludes [(triVisibleTexts), "JUEGO DE CAMPAÑA"]
triAssertIncludes [(triVisibleTexts), "OPCIONES"]
triAssertIncludes [(triVisibleTexts), "MISIÓN INDIVIDUAL"]
triAssertIncludes [(triVisibleTexts), "EDITOR DE MISIONES"]
triAssertIncludes [(triVisibleTexts), "MULTIJUGADOR"]
triAssertIncludes [(triVisibleTexts), "SALIR DEL JUEGO"]

triSetLanguage "Russian"
triAssertIncludes [(triVisibleTexts), "КАМПАНИЯ"]
triAssertIncludes [(triVisibleTexts), "НАСТРОЙКИ"]
triAssertIncludes [(triVisibleTexts), "МИССИИ"]
triAssertIncludes [(triVisibleTexts), "РЕДАКТОР МИССИЙ"]
triAssertIncludes [(triVisibleTexts), "СЕТЕВАЯ ИГРА"]
triAssertIncludes [(triVisibleTexts), "ВЫЙТИ ИЗ ИГРЫ"]

triSetLanguage "English"
triAssertIncludes [(triVisibleTexts), "CAMPAIGN GAME"]
triAssertIncludes [(triVisibleTexts), "OPTIONS"]
triAssertIncludes [(triVisibleTexts), "SINGLE MISSION"]
triAssertIncludes [(triVisibleTexts), "MISSION EDITOR"]
triAssertIncludes [(triVisibleTexts), "MULTIPLAYER"]
triAssertIncludes [(triVisibleTexts), "QUIT GAME"]

triEndTest
