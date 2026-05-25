#if defined _INC_test_features
	#endinput
#endif
#define _INC_test_features

stock Features_Hide(playerid)
{
	DX_Destroy(playerid, UI_NEW_SHADOW);
	for (new i = UI_NEW_BG; i <= UI_NEW_ICON_BELL; i++)
	{
		DX_Destroy(playerid, i);
	}
	CancelSelectTextDraw(playerid);
}

stock Features_Show(playerid)
{
	new Float:x = NewPanelX[playerid];
	new Float:y = NewPanelY[playerid];
	new Float:w = 640.0;
	new Float:h = 420.0;

	// Panel frame (Parent element)
	DX_DrawRectangle(playerid, UI_NEW_BG, x, y, w, h, 0xAA0B0D16);
	DX_SetDraggable(playerid, UI_NEW_BG, true); // Mark as draggable
	DX_SetBlurBehind(playerid, UI_NEW_BG, true); // Gorgeous glassmorphism blur!

	// Shadow element
	DX_DrawShadow(playerid, UI_NEW_SHADOW, 0.0, 0.0, w, h, 0x88000000, 20.0, 6.0);

	// Header
	DX_DrawRectangle(playerid, UI_NEW_HEADER, 0.0, 0.0, w, 50.0, 0xFF141724);
	DX_DrawText(playerid, UI_NEW_TITLE, "{3498DB}NEW {FFFFFF}ADVANCED FEATURES DEMO", 20.0, 14.0, 0xFFFFFFFF, 0.8, "Outfit-Bold");

	// Close Button
	DX_DrawButton(playerid, UI_NEW_CLOSE, w - 80.0, 10.0, 60.0, 30.0, 0xFFE74C3C, 0.55, "CLOSE", "Outfit-Bold");

	// --- Left Column ---
	// Color Picker spectrum
	DX_DrawText(playerid, UI_NEW_COLOR_LBL, "COLOR SPECTRUM PICKER:", 20.0, 70.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	new pickerColor = 0xFFFFFFFF;
	DX_GetColorPickerColor(playerid, UI_NEW_COLOR_PICKER, pickerColor);
	DX_DrawColorPicker(playerid, UI_NEW_COLOR_PICKER, 20.0, 95.0, 160.0, 80.0, pickerColor);
	
	// Small box preview of the selected color
	DX_DrawRectangle(playerid, UI_NEW_COLOR_PREVIEW, 20.0, 185.0, 160.0, 20.0, pickerColor);

	// Vector Icons demo
	DX_DrawText(playerid, UI_NEW_ICON_LBL, "VECTOR ICONS & SOUND FX:", 20.0, 220.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	DX_DrawIcon(playerid, UI_NEW_ICON_HEART, 20.0, 245.0, 28.0, "heart", 0xFFE74C3C);
	DX_DrawIcon(playerid, UI_NEW_ICON_STAR, 60.0, 245.0, 28.0, "star", 0xFFF1C40F);
	DX_DrawIcon(playerid, UI_NEW_ICON_MUSIC, 100.0, 245.0, 28.0, "music", 0xFF2ECC71);
	DX_DrawIcon(playerid, UI_NEW_ICON_BELL, 140.0, 245.0, 28.0, "bell", 0xFF9B59B6);

	// Sound Effect Button
	DX_DrawButton(playerid, UI_NEW_PLAY_SOUND, 20.0, 290.0, 160.0, 30.0, 0xFF34495E, 0.55, "PLAY SOUND FX", "Outfit-Bold");

	// --- Middle Column ---
	// Secure Password Input
	DX_DrawText(playerid, UI_NEW_PASS_LBL, "SECURE PASSWORD FIELD:", 210.0, 70.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	DX_DrawInput(playerid, UI_NEW_PASS_INPUT, 210.0, 95.0, 210.0, 30.0, 0xFFE74C3C, 0.55, "password123", "Enter password...", "Outfit-Bold");
	DX_SetInputPassword(playerid, UI_NEW_PASS_INPUT, true);

	// Slider & Circular Progress Bar
	DX_DrawText(playerid, UI_NEW_SLIDER_LBL, "PROGRESS SYNC (SLIDER):", 210.0, 145.0, 0xFF8E8E93, 0.6, "Outfit-Bold");

	new Float:slideVal = 0.5;
	if (!DX_GetSliderValue(playerid, UI_NEW_SLIDER, slideVal))
	{
		slideVal = 0.5;
	}
	DX_DrawSlider(playerid, UI_NEW_SLIDER, 210.0, 175.0, 160.0, 20.0, 0xFF3498DB, slideVal, "Outfit-Bold");

	new valText[32];
	format(valText, sizeof(valText), "%d%%", floatround(slideVal * 100.0));
	DX_DrawText(playerid, UI_NEW_SLIDER_VAL, valText, 380.0, 173.0, 0xFFFFFFFF, 0.6, "Outfit-Bold");

	// Circular Progress Bar next to the slider
	DX_DrawText(playerid, UI_NEW_CIRC_LBL, "RADIAL PROGRESS:", 210.0, 215.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	DX_DrawCircularProgress(playerid, UI_NEW_CIRC_PROGRESS, 380.0, 240.0, 20.0, slideVal, 0xFF3498DB, 5.0);

	// Vector Graph
	DX_DrawText(playerid, UI_NEW_GRAPH_LBL, "REAL-TIME VECTOR CHART:", 210.0, 275.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	new Float:graphVals[6] = {10.0, 45.0, 20.0, 80.0, 30.0, 95.0};
	graphVals[4] = slideVal * 100.0;
	DX_DrawGraph(playerid, UI_NEW_GRAPH, 210.0, 300.0, 210.0, 85.0, 0xFF2ECC71, graphVals, 6, 100.0);

	// --- Right Column ---
	// Scroll Container
	DX_DrawText(playerid, UI_NEW_SCROLL_LBL, "SCROLL CONTAINER:", 450.0, 70.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	new Float:scrollVal = 0.0;
	DX_GetScrollContainerVal(playerid, UI_NEW_SCROLL_CONT, scrollVal);
	DX_DrawScrollContainer(playerid, UI_NEW_SCROLL_CONT, 450.0, 95.0, 170.0, 160.0, 300.0, scrollVal, 0xFF1A1A2E);
	
	// Add some child elements inside the scroll container
	DX_DrawRectangle(playerid, UI_NEW_SCROLL_ITEM1, 10.0, 10.0, 130.0, 80.0, 0xFF3498DB);
	DX_DrawRectangle(playerid, UI_NEW_SCROLL_ITEM2, 10.0, 100.0, 130.0, 80.0, 0xFF2ECC71);
	DX_DrawRectangle(playerid, UI_NEW_SCROLL_ITEM3, 10.0, 190.0, 130.0, 80.0, 0xFFE74C3C);

	// Textured Progress Bar
	DX_DrawText(playerid, UI_NEW_TEX_BAR_LBL, "TEXTURED PROGRESS BAR:", 450.0, 275.0, 0xFF8E8E93, 0.6, "Outfit-Bold");
	// Textured bar synced with slider progress
	DX_DrawTexturedProgressBar(playerid, UI_NEW_TEX_BAR, 450.0, 300.0, 170.0, 25.0, "https://open.mp/favicon.ico", "https://open.mp/favicon.ico", slideVal, 0xFFFFFFFF);

	// Set parenting hierarchy
	DX_SetParent(playerid, UI_NEW_SHADOW, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_HEADER, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_TITLE, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_CLOSE, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_COLOR_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_COLOR_PICKER, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_COLOR_PREVIEW, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_ICON_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_ICON_HEART, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_ICON_STAR, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_ICON_MUSIC, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_ICON_BELL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_PLAY_SOUND, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_PASS_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_PASS_INPUT, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_SLIDER_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_SLIDER, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_SLIDER_VAL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_CIRC_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_CIRC_PROGRESS, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_GRAPH_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_GRAPH, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_SCROLL_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_SCROLL_CONT, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_SCROLL_ITEM1, UI_NEW_SCROLL_CONT);
	DX_SetParent(playerid, UI_NEW_SCROLL_ITEM2, UI_NEW_SCROLL_CONT);
	DX_SetParent(playerid, UI_NEW_SCROLL_ITEM3, UI_NEW_SCROLL_CONT);
	DX_SetParent(playerid, UI_NEW_TEX_BAR_LBL, UI_NEW_BG);
	DX_SetParent(playerid, UI_NEW_TEX_BAR, UI_NEW_BG);

	// Tooltips
	DX_SetTooltip(playerid, UI_NEW_CLOSE, "Yeni paneli kapat.");
	DX_SetTooltip(playerid, UI_NEW_PASS_INPUT, "Yazdiklarini maskeleyen guvenli alan.");
	DX_SetTooltip(playerid, UI_NEW_SLIDER, "Dairesel bari kontrol etmek icin sürükle.");
	DX_SetTooltip(playerid, UI_NEW_CIRC_PROGRESS, "Slider ile senkronize dairesel bar.");
	DX_SetTooltip(playerid, UI_NEW_COLOR_PICKER, "Renk degistirmek icin tiklayin veya surukleyin.");
	DX_SetTooltip(playerid, UI_NEW_SCROLL_CONT, "Farenin tekerlegi ile asagi yukari kaydirin.");
	DX_SetTooltip(playerid, UI_NEW_GRAPH, "Dinamik vektörel grafik çizelgesi.");
	DX_SetTooltip(playerid, UI_NEW_ICON_HEART, "Vektorel Kalp Ikonu");
	DX_SetTooltip(playerid, UI_NEW_ICON_STAR, "Vektorel Yildiz Ikonu");
	DX_SetTooltip(playerid, UI_NEW_ICON_MUSIC, "Vektorel Muzik Ikonu");
	DX_SetTooltip(playerid, UI_NEW_ICON_BELL, "Vektorel Zil Ikonu");
	DX_SetTooltip(playerid, UI_NEW_PLAY_SOUND, "Entegre ses motoruyla MP3 calmak icin tiklayin.");
}

stock bool:Features_OnClick(playerid, elementid)
{
	switch (elementid)
	{
		case UI_NEW_CLOSE:
		{
			Features_Hide(playerid);
			SendClientMessage(playerid, -1, "new test panel closed");
			return true;
		}
		case UI_NEW_PLAY_SOUND:
		{
			DX_PlaySound(playerid, "https://cdnjs.cloudflare.com/ajax/libs/ion-sound/3.0.7/sounds/button_click.mp3");
			SendClientMessage(playerid, -1, "Playing button click sound effect!");
			return true;
		}
		case UI_NEW_ICON_HEART:
		{
			DX_PlaySound(playerid, "https://cdnjs.cloudflare.com/ajax/libs/ion-sound/3.0.7/sounds/camera_flashing.mp3");
			SendClientMessage(playerid, -1, "Click sound played for Heart icon!");
			return true;
		}
		case UI_NEW_ICON_STAR:
		{
			DX_PlaySound(playerid, "https://cdnjs.cloudflare.com/ajax/libs/ion-sound/3.0.7/sounds/door_bell.mp3");
			SendClientMessage(playerid, -1, "Click sound played for Star icon!");
			return true;
		}
		case UI_NEW_ICON_MUSIC:
		{
			DX_PlaySound(playerid, "https://cdnjs.cloudflare.com/ajax/libs/ion-sound/3.0.7/sounds/pop_cork.mp3");
			SendClientMessage(playerid, -1, "Click sound played for Music icon!");
			return true;
		}
		case UI_NEW_ICON_BELL:
		{
			DX_PlaySound(playerid, "https://cdnjs.cloudflare.com/ajax/libs/ion-sound/3.0.7/sounds/bell_ring.mp3");
			SendClientMessage(playerid, -1, "Ringing bell sound effect!");
			return true;
		}
	}
	return false;
}

stock bool:Features_OnChangeSlider(playerid, elementid, Float:value)
{
	if (elementid == UI_NEW_SLIDER)
	{
		Features_Show(playerid);

		new pct = floatround(value * 100.0);
		new msg[64];
		format(msg, sizeof(msg), "new slider set to %d%%", pct);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Features_OnSelectColor(playerid, elementid, color)
{
	if (elementid == UI_NEW_COLOR_PICKER)
	{
		Features_Show(playerid);
		
		new msg[128];
		format(msg, sizeof(msg), "Selected color: 0x%08x", color);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Features_OnScroll(playerid, elementid, Float:scrollVal)
{
	if (elementid == UI_NEW_SCROLL_CONT)
	{
		new msg[64];
		format(msg, sizeof(msg), "Scroll ratio: %.2f", scrollVal);
		SendClientMessage(playerid, -1, msg);
		return true;
	}
	return false;
}

stock bool:Features_OnDrag(playerid, elementid, Float:x, Float:y)
{
	if (elementid == UI_NEW_BG)
	{
		NewPanelX[playerid] = x;
		NewPanelY[playerid] = y;
		return true;
	}
	return false;
}

stock bool:Features_OnSwapSlots(playerid, sourceElementid, targetElementid)
{
	new msg[128];
	format(msg, sizeof(msg), "Swap slots: %d <-> %d", sourceElementid, targetElementid);
	SendClientMessage(playerid, -1, msg);
	return true;
}

stock bool:Features_OnSelectRadial(playerid, elementid, index)
{
	new msg[64];
	format(msg, sizeof(msg), "Radial [ID: %d] selected item: %d", elementid, index);
	SendClientMessage(playerid, -1, msg);
	return true;
}
