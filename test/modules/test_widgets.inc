#if defined _INC_test_widgets
	#endinput
#endif
#define _INC_test_widgets

stock Widgets_Hide(playerid)
{
	DX_Destroy(playerid, UI_WIDGET_SHADOW);
	for (new i = UI_WIDGET_BG; i <= UI_WIDGET_SIDEBAR; i++)
	{
		DX_Destroy(playerid, i);
	}
	CancelSelectTextDraw(playerid);
}

stock Widgets_Show(playerid)
{
	new Float:x = WidgetPanelX[playerid];
	new Float:y = WidgetPanelY[playerid];
	new Float:w = 550.0;
	new Float:h = 420.0;

	// Panel frame (Parent element)
	DX_DrawRectangle(playerid, UI_WIDGET_BG, x, y, w, h, 0xEE0B0D16);
	DX_SetDraggable(playerid, UI_WIDGET_BG, true); // Mark as draggable

	// Shadow element
	DX_DrawShadow(playerid, UI_WIDGET_SHADOW, 0.0, 0.0, w, h, 0x55000000, 15.0, 5.0);

	// Children drawn at relative coordinates
	DX_DrawRectangle(playerid, UI_WIDGET_HEADER, 0.0, 0.0, w, 50.0, 0xFF141724);
	DX_DrawRectangle(playerid, UI_WIDGET_SIDEBAR, 0.0, 0.0, 4.0, h, 0xFF9B59B6);
	
	// Rich text rendering with colors
	DX_DrawText(playerid, UI_WIDGET_TITLE, "{9B59B6}INTERACTIVE {FFFFFF}WIDGETS DEMO", 20.0, 14.0, 0xFFFFFFFF, 0.8, "Outfit-Bold");

	// Close Button
	DX_DrawButton(playerid, UI_WIDGET_CLOSE, w - 80.0, 10.0, 60.0, 30.0, 0xFFE74C3C, 0.55, "CLOSE", "Outfit-Bold");

	// ---- Section 1: Slider (Volume Control) ----
	new Float:secY = 70.0;
	DX_DrawText(playerid, UI_WIDGET_SLIDER_LBL, "MASTER VOLUME:", 25.0, secY, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	new Float:slideVal = 0.75;
	DX_GetSliderValue(playerid, UI_WIDGET_SLIDER, slideVal);
	DX_DrawSlider(playerid, UI_WIDGET_SLIDER, 25.0, secY + 28.0, 300.0, 20.0, 0xFF3498DB, slideVal, "Outfit-Bold");
	
	new valText[32];
	format(valText, sizeof(valText), "%d%%", floatround(slideVal * 100.0));
	DX_DrawText(playerid, UI_WIDGET_SLIDER_VAL, valText, 340.0, secY + 25.0, 0xFFFFFFFF, 0.6, "Outfit-Bold");

	// ---- Section 2: ComboBox (Graphics Quality) ----
	secY += 75.0;
	DX_DrawText(playerid, UI_WIDGET_COMBO_LBL, "GRAPHICS QUALITY:", 25.0, secY, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	new comboIdx = 2;
	DX_GetComboBoxIndex(playerid, UI_WIDGET_COMBO, comboIdx);
	DX_DrawComboBox(playerid, UI_WIDGET_COMBO, 25.0, secY + 28.0, 250.0, 30.0, 0xFF2C3E50, comboIdx, "Low;Medium;High;Ultra;Custom", "Outfit-Bold");
	
	new comboText[64];
	new const qualityNames[][] = {"Low", "Medium", "High", "Ultra", "Custom"};
	format(comboText, sizeof(comboText), "{2ECC71}%s settings active", qualityNames[comboIdx]);
	DX_DrawText(playerid, UI_WIDGET_COMBO_VAL, comboText, 290.0, secY + 35.0, 0xFF2ECC71, 0.55, "Outfit-Bold");

	// ---- Section 3: ListView (Connected Players) ----
	secY += 80.0;
	DX_DrawText(playerid, UI_WIDGET_LIST_LBL, "ONLINE PLAYERS:", 25.0, secY, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	new listIdx = 0;
	DX_GetListViewIndex(playerid, UI_WIDGET_LIST, listIdx);
	DX_DrawListView(playerid, UI_WIDGET_LIST, 25.0, secY + 28.0, 250.0, 120.0, 0xFF1A1A2E, listIdx, "Player_Alpha;Player_Beta;Player_Gamma;Player_Delta;Player_Epsilon;Player_Zeta;Player_Eta;Player_Theta", "Outfit-Bold");

	// ---- Section 4: Secure Password Input ----
	DX_DrawText(playerid, UI_WIDGET_PASS_LBL, "SECURE PASSWORD:", 300.0, 70.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	DX_DrawInput(playerid, UI_WIDGET_PASS_INPUT, 300.0, 95.0, 220.0, 30.0, 0xFFE74C3C, 0.55, "qwerty", "Enter password...", "Outfit-Bold");
	DX_SetInputPassword(playerid, UI_WIDGET_PASS_INPUT, true);

	// ---- Section 5: Circular Progress Bar ----
	DX_DrawText(playerid, UI_WIDGET_CIRC_LBL, "RADIAL PROGRESS:", 300.0, 145.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	DX_DrawCircularProgress(playerid, UI_WIDGET_CIRC_PROGRESS, 410.0, 190.0, 25.0, slideVal, 0xFF3498DB, 6.0);

	// ---- Section 6: TabPanel (Settings Categories) ----
	DX_DrawText(playerid, UI_WIDGET_TAB_LBL, "SETTINGS TABS:", 300.0, 245.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	new tabIdx = 0;
	DX_GetTabActive(playerid, UI_WIDGET_TAB, tabIdx);
	DX_DrawTabPanel(playerid, UI_WIDGET_TAB, 300.0, 273.0, 220.0, 130.0, 0xFF1A1A2E, tabIdx, "General;Audio;Video", "Outfit-Bold");
	
	new tabText[64];
	new const tabNames[][] = {"General", "Audio", "Video"};
	format(tabText, sizeof(tabText), "Tab: {3498DB}%s settings active", tabNames[tabIdx]);
	DX_DrawText(playerid, UI_WIDGET_TAB_CONTENT, tabText, 310.0, 320.0, 0xFFFFFFFF, 0.55, "Outfit-Bold");

	// Parenting tree
	DX_SetParent(playerid, UI_WIDGET_SHADOW, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_HEADER, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_SIDEBAR, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_TITLE, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_CLOSE, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_SLIDER_LBL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_SLIDER, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_SLIDER_VAL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_COMBO_LBL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_COMBO, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_COMBO_VAL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_LIST_LBL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_LIST, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_TAB_LBL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_TAB, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_TAB_CONTENT, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_PASS_LBL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_PASS_INPUT, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_CIRC_LBL, UI_WIDGET_BG);
	DX_SetParent(playerid, UI_WIDGET_CIRC_PROGRESS, UI_WIDGET_BG);

	// Tooltips
	DX_SetTooltip(playerid, UI_WIDGET_CLOSE, "Kapatmak icin buraya tiklayin.");
	DX_SetTooltip(playerid, UI_WIDGET_SLIDER, "Ses seviyesini ayarlamak icin fareyi surukleyin.");
	DX_SetTooltip(playerid, UI_WIDGET_TAB, "Farkli kategori ayarlarina gecis yapin.");
	DX_SetTooltip(playerid, UI_WIDGET_COMBO, "Grafik kalitesi ayarlarini degistirmek icin tiklayin.");
	DX_SetTooltip(playerid, UI_WIDGET_LIST, "Sunucuya bagli diger oyunculari secin.");
	DX_SetTooltip(playerid, UI_WIDGET_PASS_INPUT, "Sifreli giris yapilabilen guvenli alan.");
	DX_SetTooltip(playerid, UI_WIDGET_CIRC_PROGRESS, "Master Volume ile eszamanli calisan dairesel bar.");
}

stock bool:Widgets_OnClick(playerid, elementid)
{
	if (elementid == UI_WIDGET_CLOSE)
	{
		Widgets_Hide(playerid);
		SendClientMessage(playerid, -1, "widgets closed");
		return true;
	}
	return false;
}

stock bool:Widgets_OnChangeSlider(playerid, elementid, Float:value)
{
	if (elementid == UI_WIDGET_SLIDER)
	{
		Widgets_Show(playerid);

		new pct = floatround(value * 100.0);
		new msg[64];
		format(msg, sizeof(msg), "volume set to %d%%", pct);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Widgets_OnSelectComboBox(playerid, elementid, index)
{
	if (elementid == UI_WIDGET_COMBO)
	{
		Widgets_Show(playerid);

		new msg[64];
		format(msg, sizeof(msg), "combo index: %d", index);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Widgets_OnSelectListView(playerid, elementid, index)
{
	if (elementid == UI_WIDGET_LIST)
	{
		Widgets_Show(playerid);

		new msg[64];
		format(msg, sizeof(msg), "list selection: %d", index);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Widgets_OnSelectTab(playerid, elementid, index)
{
	if (elementid == UI_WIDGET_TAB)
	{
		Widgets_Show(playerid);

		new msg[64];
		format(msg, sizeof(msg), "switched tab index %d", index);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Widgets_OnDrag(playerid, elementid, Float:x, Float:y)
{
	if (elementid == UI_WIDGET_BG)
	{
		WidgetPanelX[playerid] = x;
		WidgetPanelY[playerid] = y;
		return true;
	}
	return false;
}
