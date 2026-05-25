#if defined _INC_test_speedo
	#endinput
#endif
#define _INC_test_speedo

// Forward declare real-time update timer
forward UpdateSpeedometer(playerid);
public UpdateSpeedometer(playerid)
{
	if (!IsPlayerConnected(playerid)) return 0;

	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	// Get player actual velocity or vehicle velocity if inside a vehicle
	new Float:vx, Float:vy, Float:vz;
	if (IsPlayerInAnyVehicle(playerid))
	{
		new vehicleid = GetPlayerVehicleID(playerid);
		GetVehicleVelocity(vehicleid, vx, vy, vz);
	}
	else
	{
		GetPlayerVelocity(playerid, vx, vy, vz);
	}
	new Float:speed = floatsqroot(vx*vx + vy*vy + vz*vz) * 180.0; // Estimate speed in KM/H

	// Fuel consumption if engine is active
	if (VehicleEngine[playerid] && VehicleFuel[playerid] > 0.0)
	{
		VehicleFuel[playerid] -= 0.03;
		if (VehicleFuel[playerid] < 0.0)
		{
			VehicleFuel[playerid] = 0.0;
			VehicleEngine[playerid] = false;
			DX_DrawButton(playerid, UI_ENGINE_BTN, sw - 375.0, sh - 150.0, 100.0, 30.0, 0xFF33333F, 0.55, "ENGINE: OFF", "Outfit-Bold");
			SendClientMessage(playerid, -1, "engine stopped. no fuel left");
		}

		// Slowly increase engine temp to normal operating temp
		if (EngineTemp[playerid] < 98.0) EngineTemp[playerid] += 0.05;
	}
	else
	{
		// Cool down engine slowly when inactive
		if (EngineTemp[playerid] > 35.0) EngineTemp[playerid] -= 0.1;
	}

	// 1. Update Dial Speedometer needle (sweep of 240 degrees, from 210 to -30 degrees)
	new Float:cx = sw - 100.0;
	new Float:cy = sh - 105.0;
	new Float:radius = 50.0;

	new Float:maxSpeed = 220.0;
	new Float:ratio = speed / maxSpeed;
	if (ratio > 1.0) ratio = 1.0;
	new Float:angleDeg = 210.0 - (ratio * 240.0);
	new Float:angleRad = angleDeg * 3.14159 / 180.0;

	new Float:nx = cx + floatcos(angleRad, radian) * (radius - 10.0);
	new Float:ny = cy - floatsin(angleRad, radian) * (radius - 10.0);

	DX_DrawLine(playerid, UI_SPEED_NEEDLE, cx, cy, nx, ny, 3.5, 0xFFE74C3C); // Needle indicator

	// 2. Update digital speed text with dynamic centering offsets
	new speedStr[16];
	format(speedStr, sizeof(speedStr), "%d", floatround(speed));
	new Float:tx;
	if (speed < 10.0) tx = cx - 5.0;
	else if (speed < 100.0) tx = cx - 11.0;
	else tx = cx - 17.0;
	DX_DrawText(playerid, UI_SPEED_TEXT, speedStr, tx, cy - 10.0, 0xFFFFFFFF, 1.0, "Outfit-Bold");

	// 3. Update Fuel value bar
	new Float:fuelX = sw - 260.0;
	new Float:fuelY = sh - 130.0;
	new Float:fuelW = (VehicleFuel[playerid] / 100.0) * 90.0;
	DX_DrawLine(playerid, UI_FUEL_VAL, fuelX, fuelY, fuelX + fuelW, fuelY, 8.0, 0xFFF1C40F);

	// 4. Update Seatbelt icon color (flashes red when seatbelt is off)
	new seatbeltColor = 0xFF2ECC71; // Green if buckled
	if (!VehicleSeatbelt[playerid])
	{
		// Flash red/dark red
		if ((GetTickCount() / 250) % 2 == 0) seatbeltColor = 0xFFE74C3C;
		else seatbeltColor = 0x33E74C3C;
	}
	DX_DrawImage(playerid, UI_SEATBELT_IMG, "https://open.mp/favicon.ico", sw - 260.0, sh - 85.0, 26.0, 26.0, seatbeltColor);

	// 5. If Diagnostic Dashboard is open, update Engine Temp bar
	if (IsDashboardOpen(playerid))
	{
		new Float:dashX = (sw - 700.0) / 2.0;
		new Float:dashY = (sh - 450.0) / 2.0;

		new Float:tempX = dashX + 220.0;
		new Float:tempY = dashY + 230.0;
		new Float:tempW = (EngineTemp[playerid] / 130.0) * 200.0;
		new tempColor = 0xFF2ECC71; // Safe green
		if (EngineTemp[playerid] > 95.0) tempColor = 0xFFE67E22; // Warm orange
		if (EngineTemp[playerid] > 110.0) tempColor = 0xFFE74C3C; // Hot red
		DX_DrawLine(playerid, UI_DASH_TEMP_VAL, tempX, tempY, tempX + tempW, tempY, 8.0, tempColor);
	}
	return 1;
}

stock Speedo_Hide(playerid)
{
	DX_Destroy(playerid, UI_SPEEDOMETER_BG);
	DX_Destroy(playerid, UI_SPEED_DIAL);
	DX_Destroy(playerid, UI_SPEED_NEEDLE);
	DX_Destroy(playerid, UI_SPEED_TEXT);
	DX_Destroy(playerid, UI_SPEED_UNIT);
	DX_Destroy(playerid, UI_FUEL_BG);
	DX_Destroy(playerid, UI_FUEL_VAL);
	DX_Destroy(playerid, UI_FUEL_LABEL);
	DX_Destroy(playerid, UI_ENGINE_BTN);
	DX_Destroy(playerid, UI_LIGHTS_BTN);
	DX_Destroy(playerid, UI_LOCK_BTN);
	DX_Destroy(playerid, UI_SEATBELT_IMG);
	DX_Destroy(playerid, UI_SEATBELT_LABEL);
}

stock Speedo_Toggle(playerid)
{
	if (SpeedometerTimer[playerid] != -1)
	{
		KillTimer(SpeedometerTimer[playerid]);
		SpeedometerTimer[playerid] = -1;
		Speedo_Hide(playerid);
		SendClientMessage(playerid, -1, "speedo hidden");
	}
	else
	{
		new Float:sw, Float:sh;
		DX_GetScreenSize(playerid, sw, sh);

		new Float:cx = sw - 100.0;
		new Float:cy = sh - 105.0;
		new Float:radius = 50.0;

		// Background HUD Panel box
		DX_DrawRectangle(playerid, UI_SPEEDOMETER_BG, sw - 390.0, sh - 170.0, 360.0, 130.0, 0xAA0B0B0F);
		
		// Outer Speed dial circle
		DX_DrawCircle(playerid, UI_SPEED_DIAL, cx, cy, radius, 0xFF4A4A5A, 4.0);
		DX_DrawText(playerid, UI_SPEED_UNIT, "KM/H", cx - 16.0, cy + 18.0, 0xCC8E8E93, 0.5, "Outfit-Bold");

		// Control Buttons
		new btnColorEng = VehicleEngine[playerid] ? 0xFF2ECC71 : 0xFF33333F;
		new btnColorLgt = VehicleLights[playerid] ? 0xFF3498DB : 0xFF33333F;
		new btnColorLck = VehicleLock[playerid] ? 0xFFE67E22 : 0xFF33333F;

		DX_DrawButton(playerid, UI_ENGINE_BTN, sw - 375.0, sh - 150.0, 100.0, 30.0, btnColorEng, 0.55, VehicleEngine[playerid] ? "ENGINE: ON" : "ENGINE: OFF", "Outfit-Bold");
		DX_DrawButton(playerid, UI_LIGHTS_BTN, sw - 375.0, sh - 110.0, 100.0, 30.0, btnColorLgt, 0.55, VehicleLights[playerid] ? "LIGHTS: ON" : "LIGHTS: OFF", "Outfit-Bold");
		DX_DrawButton(playerid, UI_LOCK_BTN, sw - 375.0, sh - 70.0, 100.0, 30.0, btnColorLck, 0.55, VehicleLock[playerid] ? "VEH_DOOR: LOCK" : "VEH_DOOR: OPEN", "Outfit-Bold");

		// Fuel status
		new Float:fuelX = sw - 260.0;
		new Float:fuelY = sh - 130.0;
		DX_DrawText(playerid, UI_FUEL_LABEL, "FUEL LEVEL:", fuelX, fuelY - 20.0, 0xFF8E8E93, 0.55, "Outfit-Bold");
		DX_DrawLine(playerid, UI_FUEL_BG, fuelX, fuelY, fuelX + 90.0, fuelY, 8.0, 0xFF2A2A33);

		// Seatbelt
		DX_DrawText(playerid, UI_SEATBELT_LABEL, "BELT:", sw - 260.0, sh - 105.0, 0xFF8E8E93, 0.55, "Outfit-Bold");

		// Start timer for updates
		SpeedometerTimer[playerid] = SetTimerEx("UpdateSpeedometer", 100, true, "i", playerid);
		SendClientMessage(playerid, -1, "speedo active. use cursor to interact");
	}
	return 1;
}

stock bool:Speedo_OnClick(playerid, elementid)
{
	new Float:sw, Float:sh;
	DX_GetScreenSize(playerid, sw, sh);

	switch (elementid)
	{
		case UI_ENGINE_BTN:
		{
			VehicleEngine[playerid] = !VehicleEngine[playerid];
			new btnColor = VehicleEngine[playerid] ? 0xFF2ECC71 : 0xFF33333F;
			DX_DrawButton(playerid, UI_ENGINE_BTN, sw - 375.0, sh - 150.0, 100.0, 30.0, btnColor, 0.55, VehicleEngine[playerid] ? "ENGINE: ON" : "ENGINE: OFF", "Outfit-Bold");
			SendClientMessage(playerid, -1, VehicleEngine[playerid] ? "engine started" : "engine stopped");
			return true;
		}
		case UI_LIGHTS_BTN:
		{
			VehicleLights[playerid] = !VehicleLights[playerid];
			new btnColor = VehicleLights[playerid] ? 0xFF3498DB : 0xFF33333F;
			DX_DrawButton(playerid, UI_LIGHTS_BTN, sw - 375.0, sh - 110.0, 100.0, 30.0, btnColor, 0.55, VehicleLights[playerid] ? "LIGHTS: ON" : "LIGHTS: OFF", "Outfit-Bold");
			SendClientMessage(playerid, -1, VehicleLights[playerid] ? "lights on" : "lights off");
			return true;
		}
		case UI_LOCK_BTN:
		{
			VehicleLock[playerid] = !VehicleLock[playerid];
			new btnColor = VehicleLock[playerid] ? 0xFFE67E22 : 0xFF33333F;
			DX_DrawButton(playerid, UI_LOCK_BTN, sw - 375.0, sh - 70.0, 100.0, 30.0, btnColor, 0.55, VehicleLock[playerid] ? "VEH_DOOR: LOCK" : "VEH_DOOR: OPEN", "Outfit-Bold");
			SendClientMessage(playerid, -1, VehicleLock[playerid] ? "doors locked" : "doors unlocked");
			return true;
		}
		case UI_SEATBELT_IMG:
		{
			VehicleSeatbelt[playerid] = !VehicleSeatbelt[playerid];
			SendClientMessage(playerid, -1, VehicleSeatbelt[playerid] ? "seatbelt buckled" : "seatbelt unbuckled");
			return true;
		}
	}
	return false;
}
