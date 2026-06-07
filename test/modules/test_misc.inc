#if defined _INC_test_misc
	#endinput
#endif
#define _INC_test_misc

stock Misc_DrawImageTest(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	// Draw open.mp logo
	DX_DrawImage(playerid, 30, "https://raw.githubusercontent.com/openmultiplayer/assets/master/logo/logo.png", sw / 2.0 - 100.0, sh / 2.0 - 200.0, 200.0, 200.0, 0xFFFFFFFF);
	// Draw a tinted version next to it
	DX_DrawImage(playerid, 31, "https://raw.githubusercontent.com/openmultiplayer/assets/master/logo/logo.png", sw / 2.0 + 110.0, sh / 2.0 - 200.0, 100.0, 100.0, 0x88FF5555);

	SendClientMessage(playerid, -1, "custom images rendering");
}

stock Misc_DrawShapesTest(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	// Draw lines
	DX_DrawLine(playerid, 40, sw / 2.0 - 200.0, sw / 2.0, sw / 2.0 + 200.0, sh / 2.0 + 100.0, 8.0, 0xFFF1C40F);
	DX_DrawLine(playerid, 41, sw / 2.0 - 200.0, sw / 2.0 + 50.0, sw / 2.0 + 200.0, sh / 2.0 + 150.0, 2.0, 0xFFE74C3C);

	// Draw circles (filled and outline)
	DX_DrawCircle(playerid, 42, sw / 2.0 - 150.0, sh / 2.0 + 250.0, 60.0, 0xFF2ECC71, 0.0);
	DX_DrawCircle(playerid, 43, sw / 2.0 + 150.0, sh / 2.0 + 250.0, 60.0, 0xFF3498DB, 4.0);

	SendClientMessage(playerid, -1, "shapes rendering");
}

stock Misc_DrawClipTest(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	new Float:cx = sw / 2.0 - 100.0;
	new Float:cy = sh / 2.0 - 300.0;

	// 1. Enable clipping area
	DX_SetClipArea(playerid, 50, cx, cy, 200.0, 200.0);

	// 2. Draw a rectangle that extends beyond clip bounds
	DX_DrawRectangle(playerid, 51, cx, cy, 300.0, 300.0, 0x88E74C3C);

	// 3. Draw text extending outside
	DX_DrawText(playerid, 52, "THIS TEXT IS SO LONG THAT IT WILL BE CUT OFF BY CLIPPING", cx + 20.0, cy + 80.0, 0xFFFFFFFF, 0.8);

	// 4. Disable clipping area
	DX_SetClipArea(playerid, 53, 0.0, 0.0, 0.0, 0.0);

	// 5. Draw green rectangle outside
	DX_DrawRectangle(playerid, 54, cx - 150.0, cy, 100.0, 100.0, 0x882ECC71);

	SendClientMessage(playerid, -1, "clip active. red box clipped, green is not");
}

stock Misc_DrawShapes2Test(playerid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	new Float:cx = sw / 2.0;
	new Float:cy = sh / 2.0;

	// 1. Sleek gradient background panel
	DX_DrawGradientRectangle(playerid, 400, cx - 250.0, cy - 200.0, 500.0, 400.0, 0xEE8E44AD, 0xEE2980B9, 0xEEC0392B, 0xEED35400);

	// 2. Rounded rectangle inside card
	DX_DrawRoundedRectangle(playerid, 401, cx - 200.0, cy - 150.0, 400.0, 300.0, 15.0, 0xFA1E1E24);
	DX_DrawText(playerid, 402, "ADVANCED VECTOR SHAPES", cx - 140.0, cy - 120.0, 0xFFFFFFFF, 0.75, "Outfit");

	// 3. GPS Marker style hardware triangle
	DX_DrawTriangle(playerid, 403, cx, cy - 30.0, cx - 25.0, cy + 25.0, cx + 25.0, cy + 25.0, 0xFF3498DB);

	// Decorative gradient strips
	DX_DrawGradientRectangle(playerid, 404, cx - 160.0, cy + 70.0, 140.0, 20.0, 0xFFE74C3C, 0xFFF1C40F, 0xFFE74C3C, 0xFFF1C40F);
	DX_DrawGradientRectangle(playerid, 405, cx + 20.0, cy + 70.0, 140.0, 20.0, 0xFF2ECC71, 0xFF2ECC71, 0xFF3498DB, 0xFF3498DB);

	// Rounded button
	DX_DrawRoundedRectangle(playerid, 406, cx - 60.0, cy + 110.0, 120.0, 30.0, 8.0, 0xFFE67E22);
	DX_DrawText(playerid, 407, "SUBMIT", cx - 22.0, cy + 118.0, 0xFFFFFFFF, 0.5, "Outfit");

	SendClientMessage(playerid, -1, "shapes demo active. type /dxhideall2 to hide");
}

stock Misc_HideShapes2Test(playerid)
{
	for (new i = 400; i <= 407; i++)
	{
		DX_Destroy(playerid, i);
	}
	SendClientMessage(playerid, -1, "shapes hidden");
}

stock Misc_HideAllTest(playerid)
{
	Speedo_Hide(playerid);
	Dash_Hide(playerid);
	Widgets_Hide(playerid);
	Features_Hide(playerid);
	DX_Destroy(playerid, 30);
	DX_Destroy(playerid, 31);
	DX_Destroy(playerid, 40);
	DX_Destroy(playerid, 41);
	DX_Destroy(playerid, 42);
	DX_Destroy(playerid, 43);
	DX_Destroy(playerid, 50);
	DX_Destroy(playerid, 51);
	DX_Destroy(playerid, 52);
	DX_Destroy(playerid, 53);
	DX_Destroy(playerid, 54);
	SendClientMessage(playerid, -1, "all elements cleared");
}
