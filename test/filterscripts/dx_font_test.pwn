#include <open.mp>
#include <omp-dx>

#define DX_LAB_BASE             (9100)
#define DX_LAB_BG               (DX_LAB_BASE + 0)
#define DX_LAB_SHADOW           (DX_LAB_BASE + 1)
#define DX_LAB_HEADER           (DX_LAB_BASE + 2)
#define DX_LAB_SUBTITLE         (DX_LAB_BASE + 3)
#define DX_LAB_CLOSE            (DX_LAB_BASE + 4)
#define DX_LAB_LEFT_CARD        (DX_LAB_BASE + 5)
#define DX_LAB_RIGHT_CARD       (DX_LAB_BASE + 6)

#define DX_LAB_BTN              (DX_LAB_BASE + 10)
#define DX_LAB_CHECK            (DX_LAB_BASE + 11)
#define DX_LAB_INPUT            (DX_LAB_BASE + 12)
#define DX_LAB_PASS_INPUT       (DX_LAB_BASE + 13)
#define DX_LAB_SLIDER           (DX_LAB_BASE + 14)
#define DX_LAB_COMBO            (DX_LAB_BASE + 15)
#define DX_LAB_LIST             (DX_LAB_BASE + 16)
#define DX_LAB_TABS             (DX_LAB_BASE + 17)
#define DX_LAB_COLOR            (DX_LAB_BASE + 18)
#define DX_LAB_SCROLL           (DX_LAB_BASE + 19)
#define DX_LAB_INV_A            (DX_LAB_BASE + 20)
#define DX_LAB_INV_B            (DX_LAB_BASE + 21)
#define DX_LAB_RADIAL           (DX_LAB_BASE + 22)

#define DX_LAB_RECT             (DX_LAB_BASE + 40)
#define DX_LAB_GRAD             (DX_LAB_BASE + 41)
#define DX_LAB_ROUND            (DX_LAB_BASE + 42)
#define DX_LAB_TRI              (DX_LAB_BASE + 43)
#define DX_LAB_LINE             (DX_LAB_BASE + 44)
#define DX_LAB_CIRCLE           (DX_LAB_BASE + 45)
#define DX_LAB_IMAGE            (DX_LAB_BASE + 46)
#define DX_LAB_ICON             (DX_LAB_BASE + 47)
#define DX_LAB_CIRC             (DX_LAB_BASE + 48)
#define DX_LAB_GRAPH            (DX_LAB_BASE + 49)
#define DX_LAB_TEXBAR           (DX_LAB_BASE + 50)
#define DX_LAB_DRAG             (DX_LAB_BASE + 51)
#define DX_LAB_DRAG_TEXT        (DX_LAB_BASE + 52)
#define DX_LAB_CLIP             (DX_LAB_BASE + 53)
#define DX_LAB_CLIP_TEXT        (DX_LAB_BASE + 54)
#define DX_LAB_UNICODE_TITLE    (DX_LAB_BASE + 60)
#define DX_LAB_UNICODE_KO       (DX_LAB_BASE + 61)
#define DX_LAB_UNICODE_ZH       (DX_LAB_BASE + 62)
#define DX_LAB_UNICODE_RU       (DX_LAB_BASE + 63)
#define DX_LAB_UNICODE_TR       (DX_LAB_BASE + 64)

#define DX_LAB_BAD_HINT         (DX_LAB_BASE + 80)

#define DX_TEXT_BASE            (9300)
#define DX_TEXT_BG              (DX_TEXT_BASE + 0)
#define DX_TEXT_SHADOW          (DX_TEXT_BASE + 1)
#define DX_TEXT_TITLE           (DX_TEXT_BASE + 2)
#define DX_TEXT_SUBTITLE        (DX_TEXT_BASE + 3)
#define DX_TEXT_CLOSE           (DX_TEXT_BASE + 4)
#define DX_TEXT_CARD            (DX_TEXT_BASE + 5)
#define DX_TEXT_ROW_KO_LABEL    (DX_TEXT_BASE + 10)
#define DX_TEXT_ROW_KO          (DX_TEXT_BASE + 11)
#define DX_TEXT_ROW_ZH_LABEL    (DX_TEXT_BASE + 12)
#define DX_TEXT_ROW_ZH          (DX_TEXT_BASE + 13)
#define DX_TEXT_ROW_RU_LABEL    (DX_TEXT_BASE + 14)
#define DX_TEXT_ROW_RU          (DX_TEXT_BASE + 15)
#define DX_TEXT_ROW_TR_LABEL    (DX_TEXT_BASE + 16)
#define DX_TEXT_ROW_TR          (DX_TEXT_BASE + 17)
#define DX_TEXT_ROW_MIX_LABEL   (DX_TEXT_BASE + 18)
#define DX_TEXT_ROW_MIX         (DX_TEXT_BASE + 19)
#define DX_TEXT_SCALE_SMALL     (DX_TEXT_BASE + 20)
#define DX_TEXT_SCALE_NORMAL    (DX_TEXT_BASE + 21)
#define DX_TEXT_SCALE_BIG       (DX_TEXT_BASE + 22)
#define DX_TEXT_CLIP_AREA       (DX_TEXT_BASE + 23)
#define DX_TEXT_CLIP_TEXT       (DX_TEXT_BASE + 24)
#define DX_TEXT_HINT            (DX_TEXT_BASE + 25)

new bool:gLabCheck[MAX_PLAYERS];
new Float:gLabSlider[MAX_PLAYERS];
new gLabCombo[MAX_PLAYERS];
new gLabList[MAX_PLAYERS];
new gLabTab[MAX_PLAYERS];
new gLabColor[MAX_PLAYERS];
new Float:gLabScroll[MAX_PLAYERS];

stock DXLab_LoadFonts(playerid)
{
	DX_LoadFont(playerid, "Outfit", "Outfit.ttf");
	DX_LoadFont(playerid, "Poppins", "Poppins.ttf");
	DX_LoadFont(playerid, "JetBrains Mono", "JetBrainsMono.ttf");
	DX_LoadFont(playerid, "FontAwesome", "FontAwesome.ttf");
	return 1;
}

stock DXLab_ResetState(playerid)
{
	gLabCheck[playerid] = false;
	gLabSlider[playerid] = 0.45;
	gLabCombo[playerid] = 1;
	gLabList[playerid] = 2;
	gLabTab[playerid] = 0;
	gLabColor[playerid] = 0xFF3498DB;
	gLabScroll[playerid] = 0.20;
	return 1;
}

stock DXLab_Draw(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	DX_ClearAll(playerid);
	DXLab_LoadFonts(playerid);

	new Float:x = (sw - 920.0) / 2.0;
	new Float:y = (sh - 610.0) / 2.0;
	if (x < 20.0) x = 20.0;
	if (y < 20.0) y = 20.0;

	DX_DrawShadow(playerid, DX_LAB_SHADOW, x, y, 920.0, 610.0, 0x99000000, 24.0, 7.0);
	DX_DrawRoundedRectangle(playerid, DX_LAB_BG, x, y, 920.0, 610.0, 18.0, 0x10131AFB);
	DX_SetBlurBehind(playerid, DX_LAB_BG, true);
	DX_DrawText(playerid, DX_LAB_HEADER, "OMP-DX Component Lab", x + 28.0, y + 22.0, 0xFFFFFFFF, 1.15, "Outfit");
	DX_DrawText(playerid, DX_LAB_SUBTITLE, "Legal event test panel - F10 invalid ID spam, F11 valid ID spam", x + 28.0, y + 54.0, 0xAAB3C5FF, 0.74, "Poppins");
	DX_DrawButton(playerid, DX_LAB_CLOSE, x + 820.0, y + 22.0, 70.0, 30.0, 0xFFE74C3C, 0.55, "CLOSE", "Outfit");

	new Float:left = x + 28.0;
	new Float:right = x + 488.0;
	new Float:top = y + 92.0;

	DX_DrawRoundedRectangle(playerid, DX_LAB_LEFT_CARD, left - 12.0, top - 12.0, 430.0, 488.0, 12.0, 0x1B2130F4);
	DX_DrawRoundedRectangle(playerid, DX_LAB_RIGHT_CARD, right - 12.0, top - 12.0, 416.0, 488.0, 12.0, 0x1B2130F4);

	DX_DrawText(playerid, DX_LAB_BASE + 90, "Interactive/event components", left, top, 0xFFD6E4FFFF, 0.82, "Outfit");
	DX_DrawButton(playerid, DX_LAB_BTN, left, top + 34.0, 165.0, 32.0, 0xFF3468D8, 0.62, "Button target", "Outfit");
	DX_SetTooltip(playerid, DX_LAB_BTN, "Click test and F11 valid-ID spam target.");

	DX_DrawCheckbox(playerid, DX_LAB_CHECK, left + 190.0, top + 38.0, 22.0, 22.0, 0xFF2ECC71, gLabCheck[playerid], 0.62, "Checkbox", "Outfit");
	DX_DrawInput(playerid, DX_LAB_INPUT, left, top + 82.0, 195.0, 30.0, 0xFF3498DB, 0.58, "hello", "Input text...", "Outfit");
	DX_DrawInput(playerid, DX_LAB_PASS_INPUT, left + 215.0, top + 82.0, 195.0, 30.0, 0xFFE74C3C, 0.58, "secret", "Password...", "Outfit");
	DX_SetInputPassword(playerid, DX_LAB_PASS_INPUT, true);

	DX_DrawText(playerid, DX_LAB_BASE + 91, "Slider", left, top + 130.0, 0xFFFFFFFF, 0.62, "Outfit");
	DX_DrawSlider(playerid, DX_LAB_SLIDER, left + 76.0, top + 136.0, 250.0, 18.0, 0xFF3498DB, gLabSlider[playerid], "Outfit");
	DX_DrawComboBox(playerid, DX_LAB_COMBO, left, top + 176.0, 190.0, 30.0, 0xFF2C3E50, gLabCombo[playerid], "Low;Medium;High;Ultra", "Outfit");
	DX_DrawListView(playerid, DX_LAB_LIST, left + 220.0, top + 176.0, 190.0, 96.0, 0xFF111722, gLabList[playerid], "Alpha;Beta;Gamma;Delta;Epsilon;Zeta", "Outfit");
	DX_DrawTabPanel(playerid, DX_LAB_TABS, left, top + 238.0, 190.0, 88.0, 0xFF111722, gLabTab[playerid], "General;Audio;Video", "Outfit");
	DX_DrawColorPicker(playerid, DX_LAB_COLOR, left + 220.0, top + 298.0, 170.0, 78.0, gLabColor[playerid]);
	DX_DrawScrollContainer(playerid, DX_LAB_SCROLL, left, top + 356.0, 190.0, 82.0, 240.0, gLabScroll[playerid], 0xFF111722);
	DX_DrawInventorySlot(playerid, DX_LAB_INV_A, left + 220.0, top + 404.0, 50.0, 50.0, 0xFF263245, "https://open.mp/favicon.ico", "A", 3);
	DX_DrawInventorySlot(playerid, DX_LAB_INV_B, left + 280.0, top + 404.0, 50.0, 50.0, 0xFF263245, "https://open.mp/favicon.ico", "B", 7);
	DX_DrawRadialMenu(playerid, DX_LAB_RADIAL, left + 372.0, top + 430.0, 34.0, 0xAA3468D8, -1, "Use;Drop;Info", "check;trash;info");

	DX_DrawText(playerid, DX_LAB_BASE + 92, "Visual/render components", right, top, 0xFFD6E4FFFF, 0.82, "Outfit");
	DX_DrawRectangle(playerid, DX_LAB_RECT, right, top + 36.0, 86.0, 34.0, 0xFF34495E);
	DX_DrawGradientRectangle(playerid, DX_LAB_GRAD, right + 106.0, top + 36.0, 118.0, 34.0, 0xFFE74C3C, 0xFFF1C40F, 0xFF8E44AD, 0xFF3498DB);
	DX_DrawRoundedRectangle(playerid, DX_LAB_ROUND, right + 244.0, top + 36.0, 122.0, 34.0, 10.0, 0xFF2ECC71);
	DX_DrawTriangle(playerid, DX_LAB_TRI, right + 410.0, top + 34.0, right + 384.0, top + 76.0, right + 436.0, top + 76.0, 0xFF9B59B6);
	DX_DrawLine(playerid, DX_LAB_LINE, right, top + 108.0, right + 220.0, top + 108.0, 5.0, 0xFFF1C40F);
	DX_DrawCircle(playerid, DX_LAB_CIRCLE, right + 292.0, top + 108.0, 30.0, 0xFF3498DB, 4.0);
	DX_DrawImage(playerid, DX_LAB_IMAGE, "https://open.mp/favicon.ico", right + 366.0, top + 82.0, 54.0, 54.0, 0xFFFFFFFF);
	DX_DrawIcon(playerid, DX_LAB_ICON, right, top + 156.0, 28.0, "heart", 0xFFE74C3C);
	DX_DrawCircularProgress(playerid, DX_LAB_CIRC, right + 88.0, top + 172.0, 26.0, gLabSlider[playerid], 0xFF3498DB, 6.0);

	new Float:graphVals[6];
	graphVals[0] = 20.0;
	graphVals[1] = 42.0;
	graphVals[2] = 35.0;
	graphVals[3] = 68.0;
	graphVals[4] = 54.0;
	graphVals[5] = 88.0;
	DX_DrawGraph(playerid, DX_LAB_GRAPH, right + 150.0, top + 150.0, 220.0, 88.0, 0xFF2ECC71, graphVals, 6, 100.0);
	DX_DrawTexturedProgressBar(playerid, DX_LAB_TEXBAR, right, top + 264.0, 220.0, 24.0, "https://open.mp/favicon.ico", "https://open.mp/favicon.ico", gLabSlider[playerid], 0xFFFFFFFF);
	DX_DrawRoundedRectangle(playerid, DX_LAB_DRAG, right + 250.0, top + 260.0, 120.0, 44.0, 8.0, 0xFF34495E);
	DX_DrawText(playerid, DX_LAB_DRAG_TEXT, "Drag me", right + 274.0, top + 272.0, 0xFFFFFFFF, 0.62, "Outfit");
	DX_SetDraggable(playerid, DX_LAB_DRAG, true);
	DX_SetTooltip(playerid, DX_LAB_DRAG, "Draggable rounded rectangle.");
	DX_SetClipArea(playerid, DX_LAB_CLIP, right, top + 328.0, 250.0, 48.0);
	DX_DrawText(playerid, DX_LAB_CLIP_TEXT, "Clip area: this long text should be clipped inside the component bounds.", right, top + 338.0, 0xFFFFFFFF, 0.62, "Outfit");
	DX_DrawText(playerid, DX_LAB_UNICODE_TITLE, "Unicode text test", right, top + 394.0, 0xFFD6E4FFFF, 0.66, "Outfit");
	DX_DrawText(playerid, DX_LAB_UNICODE_KO, "한국어: 안녕하세요, open.mp DX", right, top + 420.0, 0xFFFFFFFF, 0.58, "Malgun Gothic");
	DX_DrawText(playerid, DX_LAB_UNICODE_ZH, "中文: 你好，世界，测试文本", right, top + 444.0, 0xFFFFFFFF, 0.58, "Microsoft YaHei");
	DX_DrawText(playerid, DX_LAB_UNICODE_RU, "Русский: Привет, мир, проверка", right, top + 468.0, 0xFFFFFFFF, 0.58, "Arial");
	DX_DrawText(playerid, DX_LAB_UNICODE_TR, "Türkçe: ÇĞİÖŞÜ çğıöşü ıİ", right, top + 492.0, 0xFFFFFFFF, 0.58, "Arial");

	SelectTextDraw(playerid, 0x66B7FFD6);
	SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] Component lab opened. Use F10/F11 for spam-guard tests.");
	return 1;
}

stock DXLab_BadFont(playerid)
{
	DX_LoadFont(playerid, "Rejected Font", "RejectedFont.ttf");
	DX_DrawText(playerid, DX_LAB_BAD_HINT, "Requested RejectedFont.ttf. Expected: client rejects by allowlist.", 260.0, 620.0, 0xFF8E8EFF, 0.82, "Outfit");
	SendClientMessage(playerid, 0xFF8E8EFF, "[omp-dx] Requested RejectedFont.ttf. It should be rejected by client allowlist.");
	return 1;
}

stock DXText_Draw(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	DX_ClearAll(playerid);
	DXLab_LoadFonts(playerid);

	new Float:x = (sw - 860.0) / 2.0;
	new Float:y = (sh - 520.0) / 2.0;
	if (x < 20.0) x = 20.0;
	if (y < 20.0) y = 20.0;

	DX_DrawShadow(playerid, DX_TEXT_SHADOW, x, y, 860.0, 520.0, 0x99000000, 24.0, 7.0);
	DX_DrawRoundedRectangle(playerid, DX_TEXT_BG, x, y, 860.0, 520.0, 18.0, 0x111722F8);
	DX_DrawRoundedRectangle(playerid, DX_TEXT_CARD, x + 28.0, y + 92.0, 804.0, 336.0, 12.0, 0x202A3BF4);
	DX_DrawText(playerid, DX_TEXT_TITLE, "OMP-DX Text Rendering Lab", x + 30.0, y + 24.0, 0xFFFFFFFF, 1.16, "Outfit");
	DX_DrawText(playerid, DX_TEXT_SUBTITLE, "Command: /dxtext - Unicode, font coverage, scale and clipping test", x + 32.0, y + 58.0, 0xAAB3C5FF, 0.66, "Poppins");
	DX_DrawButton(playerid, DX_TEXT_CLOSE, x + 758.0, y + 24.0, 74.0, 30.0, 0xFFE74C3C, 0.55, "CLOSE", "Outfit");

	new Float:left = x + 52.0;
	new Float:textX = x + 214.0;
	new Float:top = y + 118.0;

	DX_DrawText(playerid, DX_TEXT_ROW_KO_LABEL, "Korean / Malgun Gothic", left, top, 0xFF8ED1FC, 0.58, "Outfit");
	DX_DrawText(playerid, DX_TEXT_ROW_KO, "한국어: 안녕하세요, 오픈엠피 DX 텍스트 테스트", textX, top, 0xFFFFFFFF, 0.70, "Malgun Gothic");

	DX_DrawText(playerid, DX_TEXT_ROW_ZH_LABEL, "Chinese / Microsoft YaHei", left, top + 48.0, 0xFF8ED1FC, 0.58, "Outfit");
	DX_DrawText(playerid, DX_TEXT_ROW_ZH, "中文: 你好，世界，字体渲染测试", textX, top + 48.0, 0xFFFFFFFF, 0.70, "Microsoft YaHei");

	DX_DrawText(playerid, DX_TEXT_ROW_RU_LABEL, "Russian / Arial", left, top + 96.0, 0xFF8ED1FC, 0.58, "Outfit");
	DX_DrawText(playerid, DX_TEXT_ROW_RU, "Русский: Привет, мир, проверка текста", textX, top + 96.0, 0xFFFFFFFF, 0.70, "Arial");

	DX_DrawText(playerid, DX_TEXT_ROW_TR_LABEL, "Turkish / Arial", left, top + 144.0, 0xFF8ED1FC, 0.58, "Outfit");
	DX_DrawText(playerid, DX_TEXT_ROW_TR, "Türkçe: ÇĞİÖŞÜ çğıöşü ıİ düzgün mü?", textX, top + 144.0, 0xFFFFFFFF, 0.70, "Arial");

	DX_DrawText(playerid, DX_TEXT_ROW_MIX_LABEL, "Mixed / Poppins", left, top + 192.0, 0xFF8ED1FC, 0.58, "Outfit");
	DX_DrawText(playerid, DX_TEXT_ROW_MIX, "Latin + Türkçe + Русский + 中文 + 한국어", textX, top + 192.0, 0xFFFFFFFF, 0.70, "Poppins");

	DX_DrawText(playerid, DX_TEXT_SCALE_SMALL, "Small 0.45: abc ABC 123 ÇĞİ Рус 中文", left, y + 456.0, 0xFFD6E4FFFF, 0.45, "Arial");
	DX_DrawText(playerid, DX_TEXT_SCALE_NORMAL, "Normal 0.75: abc ABC 123 ÇĞİ Рус 中文", left + 260.0, y + 456.0, 0xFFFFFFFF, 0.75, "Arial");
	DX_DrawText(playerid, DX_TEXT_SCALE_BIG, "Big 1.05", left + 610.0, y + 448.0, 0xFFFFD166, 1.05, "Outfit");

	DX_SetClipArea(playerid, DX_TEXT_CLIP_AREA, x + 52.0, y + 396.0, 520.0, 30.0);
	DX_DrawText(playerid, DX_TEXT_CLIP_TEXT, "Clip test: Bu uzun satır kutunun sonunda kesilmeli - 한국어 中文 Русский Türkçe", x + 52.0, y + 400.0, 0xFFFFFFFF, 0.58, "Arial");
	DX_DrawText(playerid, DX_TEXT_HINT, "Expected: no mojibake, no missing squares, Turkish dotted/dotless I correct.", x + 52.0, y + 430.0, 0xAAB3C5FF, 0.52, "Poppins");

	SelectTextDraw(playerid, 0x66B7FFD6);
	SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] Text rendering lab opened. Check Korean/Chinese/Russian/Turkish rows.");
	return 1;
}

public OnFilterScriptInit()
{
	print("[omp-dx] dx component lab filterscript loaded.");
	return 1;
}

public OnPlayerConnect(playerid)
{
	DXLab_ResetState(playerid);
	SendClientMessage(playerid, 0xD6E4FFFF, "[omp-dx] Use /dxlab for components or /dxtext for text rendering.");
	return 1;
}

public OnPlayerDXReady(playerid)
{
	DXLab_Draw(playerid);
	return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
	if (!strcmp(cmdtext, "/dxlab", true) || !strcmp(cmdtext, "/dxfont", true))
	{
		DXLab_Draw(playerid);
		return 1;
	}

	if (!strcmp(cmdtext, "/dxtext", true))
	{
		DXText_Draw(playerid);
		return 1;
	}

	if (!strcmp(cmdtext, "/dxfontbad", true))
	{
		DXLab_BadFont(playerid);
		return 1;
	}

	if (!strcmp(cmdtext, "/dxclear", true))
	{
		DX_ClearAll(playerid);
		CancelSelectTextDraw(playerid);
		SendClientMessage(playerid, 0xAAB3C5FF, "[omp-dx] Cleared all DX elements.");
		return 1;
	}

	return 0;
}

public OnPlayerClickDX(playerid, elementid)
{
	if (elementid == DX_LAB_CLOSE)
	{
		DX_ClearAll(playerid);
		CancelSelectTextDraw(playerid);
		SendClientMessage(playerid, 0xAAB3C5FF, "[omp-dx] Component lab closed.");
		return 1;
	}
	if (elementid == DX_TEXT_CLOSE)
	{
		DX_ClearAll(playerid);
		CancelSelectTextDraw(playerid);
		SendClientMessage(playerid, 0xAAB3C5FF, "[omp-dx] Text rendering lab closed.");
		return 1;
	}
	if (elementid == DX_LAB_BTN)
	{
		SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] Button callback OK.");
		return 1;
	}
	return 1;
}

public OnPlayerToggleDXCheckbox(playerid, elementid, bool:checked)
{
	if (elementid == DX_LAB_CHECK)
	{
		gLabCheck[playerid] = checked;
		SendClientMessage(playerid, 0xB7FFD6FF, checked ? "[omp-dx] Checkbox ON." : "[omp-dx] Checkbox OFF.");
	}
	return 1;
}

public OnPlayerDXInputSubmit(playerid, elementid, const text[])
{
	if (elementid == DX_LAB_INPUT || elementid == DX_LAB_PASS_INPUT)
	{
		new msg[144];
		format(msg, sizeof msg, "[omp-dx] Input submit element=%d text=%s", elementid, text);
		SendClientMessage(playerid, 0xB7FFD6FF, msg);
	}
	return 1;
}

public OnPlayerChangeDXSlider(playerid, elementid, Float:value)
{
	if (elementid == DX_LAB_SLIDER)
	{
		new Float:sw, Float:sh;
		DX_GetScreenSize(playerid, sw, sh);
		new Float:x = (sw - 920.0) / 2.0;
		new Float:y = (sh - 610.0) / 2.0;
		if (x < 20.0) x = 20.0;
		if (y < 20.0) y = 20.0;
		gLabSlider[playerid] = value;
		DX_DrawCircularProgress(playerid, DX_LAB_CIRC, x + 576.0, y + 172.0, 26.0, value, 0xFF3498DB, 6.0);
	}
	return 1;
}

public OnPlayerSelectDXComboBox(playerid, elementid, index)
{
	if (elementid == DX_LAB_COMBO)
	{
		gLabCombo[playerid] = index;
		SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] ComboBox callback OK.");
	}
	return 1;
}

public OnPlayerSelectDXListView(playerid, elementid, index)
{
	if (elementid == DX_LAB_LIST)
	{
		gLabList[playerid] = index;
		SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] ListView callback OK.");
	}
	return 1;
}

public OnPlayerSelectDXTab(playerid, elementid, index)
{
	if (elementid == DX_LAB_TABS)
	{
		gLabTab[playerid] = index;
		SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] Tab callback OK.");
	}
	return 1;
}

public OnPlayerSelectDXColor(playerid, elementid, color)
{
	if (elementid == DX_LAB_COLOR)
	{
		gLabColor[playerid] = color;
	}
	return 1;
}

public OnPlayerScrollDXContainer(playerid, elementid, Float:scrollVal)
{
	if (elementid == DX_LAB_SCROLL)
	{
		gLabScroll[playerid] = scrollVal;
	}
	return 1;
}

public OnPlayerDragDX(playerid, elementid, Float:x, Float:y)
{
	if (elementid == DX_LAB_DRAG)
	{
		DX_DrawRoundedRectangle(playerid, DX_LAB_DRAG, x, y, 120.0, 44.0, 8.0, 0xFF34495E);
		DX_DrawText(playerid, DX_LAB_DRAG_TEXT, "Drag me", x + 24.0, y + 12.0, 0xFFFFFFFF, 0.62, "Outfit");
		DX_SetDraggable(playerid, DX_LAB_DRAG, true);
	}
	return 1;
}

public OnPlayerSwapDXSlots(playerid, sourceElementid, targetElementid)
{
	new msg[128];
	format(msg, sizeof msg, "[omp-dx] Inventory swap callback OK: %d -> %d", sourceElementid, targetElementid);
	SendClientMessage(playerid, 0xB7FFD6FF, msg);
	return 1;
}

public OnPlayerSelectRadialItem(playerid, elementid, index)
{
	if (elementid == DX_LAB_RADIAL)
	{
		SendClientMessage(playerid, 0xB7FFD6FF, "[omp-dx] Radial menu callback OK.");
	}
	return 1;
}
