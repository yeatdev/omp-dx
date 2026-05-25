#if defined _INC_test_dash
	#endinput
#endif
#define _INC_test_dash

stock Dash_Hide(playerid)
{
	DX_Destroy(playerid, UI_DASH_BG);
	DX_Destroy(playerid, UI_DASH_HEADER);
	DX_Destroy(playerid, 150);
	DX_Destroy(playerid, 151);
	DX_Destroy(playerid, 152);
	DX_Destroy(playerid, UI_DASH_TITLE);
	
	DX_Destroy(playerid, UI_DASH_MAP_IMG);
	DX_Destroy(playerid, UI_DASH_MAP_RING);
	DX_Destroy(playerid, UI_DASH_COMPASS_N);
	DX_Destroy(playerid, UI_DASH_COMPASS_S);
	
	DX_Destroy(playerid, UI_DASH_TEMP_LABEL);
	DX_Destroy(playerid, UI_DASH_TEMP_BG);
	DX_Destroy(playerid, UI_DASH_TEMP_VAL);
	
	DX_Destroy(playerid, UI_DASH_LIMITER_LABEL);
	DX_Destroy(playerid, UI_DASH_LIMITER_INPUT);
	DX_Destroy(playerid, UI_DASH_SAVE);
	DX_Destroy(playerid, UI_DASH_CLOSE);
	DX_Destroy(playerid, UI_DASH_SIDEBAR);
	
	DX_Destroy(playerid, UI_DASH_CLIP_START);
	DX_Destroy(playerid, UI_DASH_LOG_1_BG);
	DX_Destroy(playerid, UI_DASH_LOG_1_TXT);
	DX_Destroy(playerid, UI_DASH_LOG_2_BG);
	DX_Destroy(playerid, UI_DASH_LOG_2_TXT);
	DX_Destroy(playerid, UI_DASH_LOG_3_BG);
	DX_Destroy(playerid, UI_DASH_LOG_3_TXT);
	DX_Destroy(playerid, UI_DASH_LOG_4_BG);
	DX_Destroy(playerid, UI_DASH_LOG_4_TXT);
	DX_Destroy(playerid, UI_DASH_CLIP_END);
	CancelSelectTextDraw(playerid);
}

stock Dash_Show(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	// Layout: 700x420 Diagnostics screen in the center of the display
	new Float:w = 700.0;
	new Float:h = 420.0;
	new Float:x = (sw - w) / 2.0;
	new Float:y = (sh - h) / 2.0;

	// 1. Draw dashboard frame
	DX_DrawRectangle(playerid, UI_DASH_BG, x, y, w, h, 0xEE0B0D16);
	DX_DrawRectangle(playerid, UI_DASH_HEADER, x, y, w, 55.0, 0xFF141724);
	DX_DrawRectangle(playerid, UI_DASH_SIDEBAR, x + w - 30.0, y, 30.0, h, 0xFF3498DB); // Thin decoration sidebar
	DX_DrawText(playerid, UI_DASH_TITLE, "VEHICLE DIAGNOSTICS & GPS LINK", x + 25.0, y + 16.0, 0xFFFFFFFF, 0.8, "Outfit-Bold");

	// Left Column: GPS Map Link preview (Compass and web avatar map image)
	new Float:leftColX = x + 30.0;
	new Float:leftColY = y + 80.0;
	DX_DrawText(playerid, 150, "GPS LINK PREVIEW:", leftColX, leftColY, 0xFF8E8E93, 0.6, "Outfit-Bold");
	
	// Map circle preview
	DX_DrawImage(playerid, UI_DASH_MAP_IMG, "https://open.mp/favicon.ico", leftColX + 15.0, leftColY + 30.0, 110.0, 110.0, 0xFFFFFFFF);
	DX_DrawCircle(playerid, UI_DASH_MAP_RING, leftColX + 70.0, leftColY + 85.0, 60.0, 0xFF3498DB, 3.0);
	
	// Compass Direction points
	DX_DrawText(playerid, UI_DASH_COMPASS_N, "N", leftColX + 65.0, leftColY + 12.0, 0xFFE74C3C, 0.6, "Outfit-Bold");
	DX_DrawText(playerid, UI_DASH_COMPASS_S, "S", leftColX + 66.0, leftColY + 150.0, 0xFF8E8E93, 0.6, "Outfit-Bold");

	// Middle Column: Live Diagnostics Stats & Limiter Input
	new Float:midColX = x + 210.0;
	new Float:midColY = y + 80.0;
	DX_DrawText(playerid, 151, "ENGINE & LIMITER PREFERENCES", midColX, midColY, 0xFF8E8E93, 0.6, "Outfit-Bold");

	// Engine Temp bar label
	DX_DrawText(playerid, UI_DASH_TEMP_LABEL, "ENGINE CORE TEMP:", midColX, midColY + 40.0, 0xFFFFFFFF, 0.6, "Outfit-Bold");
	DX_DrawLine(playerid, UI_DASH_TEMP_BG, midColX, midColY + 70.0, midColX + 200.0, midColY + 70.0, 8.0, 0xFF2A2A33);

	// Speed limiter input field
	new limStr[16];
	format(limStr, sizeof(limStr), "%d", SpeedLimiterVal[playerid]);
	DX_DrawText(playerid, UI_DASH_LIMITER_LABEL, "CRUISE SPEED LIMIT (KM/H):", midColX, midColY + 110.0, 0xFFFFFFFF, 0.6, "Outfit-Bold");
	DX_DrawInput(playerid, UI_DASH_LIMITER_INPUT, midColX, midColY + 140.0, 200.0, 40.0, 0xFF3498DB, 0.65, limStr, "Enter speed limit...", "Outfit-Bold");

	// Save and Close buttons
	DX_DrawButton(playerid, UI_DASH_SAVE, midColX, midColY + 230.0, 95.0, 45.0, 0xFF2ECC71, 0.7, "Apply", "Outfit-Bold");
	DX_DrawButton(playerid, UI_DASH_CLOSE, midColX + 110.0, midColY + 230.0, 90.0, 45.0, 0xFFE74C3C, 0.7, "Close", "Outfit-Bold");

	// Right Column: Hardware Scissor-Clipped Live Alert Feed
	new Float:rightColX = x + 470.0;
	new Float:rightColY = y + 80.0;
	DX_DrawText(playerid, 152, "DIAGNOSTIC EVENTS FEED", rightColX, rightColY, 0xFF8E8E93, 0.6, "Outfit-Bold");

	// Scissor Clip Box: width 180, height 230
	DX_SetClipArea(playerid, UI_DASH_CLIP_START, rightColX, rightColY + 30.0, 180.0, 230.0);

	// Event Item 1
	DX_DrawRectangle(playerid, UI_DASH_LOG_1_BG, rightColX, rightColY + 30.0, 180.0, 50.0, 0x22FFFFFF);
	DX_DrawText(playerid, UI_DASH_LOG_1_TXT, "[LOG] Engine link online", rightColX + 8.0, rightColY + 45.0, 0xFFFFFFFF, 0.6, "Outfit-Bold");

	// Event Item 2
	DX_DrawRectangle(playerid, UI_DASH_LOG_2_BG, rightColX, rightColY + 90.0, 180.0, 50.0, 0x22FFFFFF);
	DX_DrawText(playerid, UI_DASH_LOG_2_TXT, "[LOG] GPS signal locks", rightColX + 8.0, rightColY + 105.0, 0xFF2ECC71, 0.6, "Outfit-Bold");

	// Event Item 3
	DX_DrawRectangle(playerid, UI_DASH_LOG_3_BG, rightColX, rightColY + 150.0, 180.0, 50.0, 0x22FFFFFF);
	DX_DrawText(playerid, UI_DASH_LOG_3_TXT, "[LOG] Fuel sensor ready", rightColX + 8.0, rightColY + 165.0, 0xFF3498DB, 0.6, "Outfit-Bold");

	// Event Item 4
	DX_DrawRectangle(playerid, UI_DASH_LOG_4_BG, rightColX, rightColY + 210.0, 180.0, 50.0, 0x22E74C3C);
	DX_DrawText(playerid, UI_DASH_LOG_4_TXT, "[LOG] CORE OVERTEMP WARNING", rightColX + 8.0, rightColY + 225.0, 0xFFE74C3C, 0.6, "Outfit-Bold");

	// End Clip Area
	DX_SetClipArea(playerid, UI_DASH_CLIP_END, 0.0, 0.0, 0.0, 0.0);

	SelectTextDraw(playerid, 0xFF0000FF);
	SendClientMessage(playerid, -1, "diagnostics open. use cursor");
}

stock bool:Dash_OnClick(playerid, elementid)
{
	switch (elementid)
	{
		case UI_DASH_SAVE:
		{
			new limitStr[32];
			DX_GetInputText(playerid, UI_DASH_LIMITER_INPUT, limitStr, sizeof(limitStr));
			new limit = strval(limitStr);
			if (limit > 0)
			{
				SpeedLimiterVal[playerid] = limit;
				new msg[128];
				format(msg, sizeof(msg), "speed limiter set to %d", limit);
				SendClientMessage(playerid, -1, msg);
			}
			Dash_Hide(playerid);
			return true;
		}
		case UI_DASH_CLOSE:
		{
			Dash_Hide(playerid);
			return true;
		}
	}
	return false;
}

stock bool:Dash_OnInputSubmit(playerid, elementid, const text[])
{
	if (elementid == UI_DASH_LIMITER_INPUT)
	{
		new limit = strval(text);
		if (limit > 0)
		{
			SpeedLimiterVal[playerid] = limit;
			new msg[128];
			format(msg, sizeof(msg), "limiter set to %d", limit);
			SendClientMessage(playerid, -1, msg);
		}
		return true;
	}
	return false;
}
