#include <open.mp>
#include <float>
#include <omp-dx>

// Include modular test inclusions
#include "modules/test_defs.inc"
#include "modules/test_speedo.inc"
#include "modules/test_dash.inc"
#include "modules/test_widgets.inc"
#include "modules/test_features.inc"
#include "modules/test_misc.inc"

// ==================== MAIN ENTRY CALLBACKS ====================

public OnPlayerConnect(playerid)
{
	SpeedometerTimer[playerid] = -1;
	VehicleEngine[playerid] = false;
	VehicleLights[playerid] = false;
	VehicleLock[playerid] = false;
	VehicleSeatbelt[playerid] = false;
	VehicleFuel[playerid] = 85.0;
	EngineTemp[playerid] = 90.0;
	SpeedLimiterVal[playerid] = 120;
	return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
	if (SpeedometerTimer[playerid] != -1)
	{
		KillTimer(SpeedometerTimer[playerid]);
		SpeedometerTimer[playerid] = -1;
	}
	return 1;
}

public OnPlayerSpawn(playerid)
{
	SendClientMessage(playerid, -1, "speedo and dash loaded. use /dxspeed or /dxshow");
	
	// Load premium Outfit font asynchronously
	DX_LoadFont(playerid, "Outfit-Bold", "https://raw.githubusercontent.com/google/fonts/main/ofl/outfit/Outfit-Bold.ttf");
	return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
	if (strcmp(cmdtext, "/dximage", true) == 0)
	{
		Misc_DrawImageTest(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxshapes", true) == 0)
	{
		Misc_DrawShapesTest(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxclip", true) == 0)
	{
		Misc_DrawClipTest(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxhideall", true) == 0)
	{
		Misc_HideAllTest(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxspeed", true) == 0)
	{
		Speedo_Toggle(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxshow", true) == 0)
	{
		Dash_Show(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxhide", true) == 0)
	{
		Dash_Hide(playerid);
		SendClientMessage(playerid, -1, "dashboard closed");
		return 1;
	}

	if (strcmp(cmdtext, "/dxshapes2", true) == 0)
	{
		Misc_DrawShapes2Test(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxhideall2", true) == 0)
	{
		Misc_HideShapes2Test(playerid);
		return 1;
	}

	if (strcmp(cmdtext, "/dxwidgets", true) == 0)
	{
		new Float:sw, Float:sh;
		DX_GetScreenSize(playerid, sw, sh);

		new Float:w = 550.0;
		new Float:h = 420.0;
		WidgetPanelX[playerid] = (sw - w) / 2.0;
		WidgetPanelY[playerid] = (sh - h) / 2.0;

		Widgets_Show(playerid);
		SelectTextDraw(playerid, 0xFF0000FF);
		SendClientMessage(playerid, -1, "widgets open. use cursor to interact");
		return 1;
	}

	if (strcmp(cmdtext, "/dxhidewidgets", true) == 0)
	{
		Widgets_Hide(playerid);
		SendClientMessage(playerid, -1, "widgets closed");
		return 1;
	}

	if (strcmp(cmdtext, "/dxnew", true) == 0)
	{
		new Float:sw, Float:sh;
		DX_GetScreenSize(playerid, sw, sh);

		new Float:w = 640.0;
		new Float:h = 420.0;
		NewPanelX[playerid] = (sw - w) / 2.0;
		NewPanelY[playerid] = (sh - h) / 2.0;

		Features_Show(playerid);
		SelectTextDraw(playerid, 0xFF0000FF);
		SendClientMessage(playerid, -1, "new test panel open. use cursor to interact");
		return 1;
	}

	if (strcmp(cmdtext, "/dxhidenew", true) == 0)
	{
		Features_Hide(playerid);
		SendClientMessage(playerid, -1, "new test panel closed");
		return 1;
	}

	return 0;
}

// Interactive Callbacks routed to specific module layers
public OnPlayerClickDX(playerid, elementid)
{
	if (Speedo_OnClick(playerid, elementid)) return 1;
	if (Dash_OnClick(playerid, elementid)) return 1;
	if (Widgets_OnClick(playerid, elementid)) return 1;
	if (Features_OnClick(playerid, elementid)) return 1;
	return 1;
}

public OnPlayerToggleDXCheckbox(playerid, elementid, bool:checked)
{
	return 1;
}

public OnPlayerDXInputSubmit(playerid, elementid, const text[])
{
	if (Dash_OnInputSubmit(playerid, elementid, text)) return 1;
	return 1;
}

public OnPlayerChangeDXSlider(playerid, elementid, Float:value)
{
	if (Widgets_OnChangeSlider(playerid, elementid, value)) return 1;
	if (Features_OnChangeSlider(playerid, elementid, value)) return 1;
	return 1;
}

public OnPlayerSelectDXComboBox(playerid, elementid, index)
{
	if (Widgets_OnSelectComboBox(playerid, elementid, index)) return 1;
	return 1;
}

public OnPlayerSelectDXListView(playerid, elementid, index)
{
	if (Widgets_OnSelectListView(playerid, elementid, index)) return 1;
	return 1;
}

public OnPlayerSelectDXTab(playerid, elementid, index)
{
	if (Widgets_OnSelectTab(playerid, elementid, index)) return 1;
	return 1;
}

public OnPlayerDragDX(playerid, elementid, Float:x, Float:y)
{
	if (Widgets_OnDrag(playerid, elementid, x, y)) return 1;
	if (Features_OnDrag(playerid, elementid, x, y)) return 1;
	return 1;
}

public OnPlayerSelectDXColor(playerid, elementid, color)
{
	if (Features_OnSelectColor(playerid, elementid, color)) return 1;
	return 1;
}

public OnPlayerScrollDXContainer(playerid, elementid, Float:scrollVal)
{
	if (Features_OnScroll(playerid, elementid, scrollVal)) return 1;
	return 1;
}

public OnPlayerSwapDXSlots(playerid, sourceElementid, targetElementid)
{
	if (Features_OnSwapSlots(playerid, sourceElementid, targetElementid)) return 1;
	return 1;
}

public OnPlayerSelectRadialItem(playerid, elementid, index)
{
	if (Features_OnSelectRadial(playerid, elementid, index)) return 1;
	return 1;
}
