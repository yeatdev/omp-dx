#if defined _INC_dx_renderer
	#endinput
#endif
#define _INC_dx_renderer

native bool:DX_DrawRectangle(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color);
native bool:DX_DrawText(playerid, elementid, const text[], Float:x, Float:y, color, Float:scale, const font[] = "");
native bool:DX_Destroy(playerid, elementid);
native bool:DX_ClearAll(playerid);
native bool:DX_GetScreenSize(playerid, &Float:width, &Float:height);

native bool:DX_DrawButton(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:scale, const text[], const font[] = "");
native bool:DX_DrawCheckbox(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, bool:checked, Float:scale, const label[], const font[] = "");
native bool:DX_DrawInput(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:scale, const defaultText[], const placeholder[], const font[] = "");
native bool:DX_GetCheckboxState(playerid, elementid, &bool:checked);
native bool:DX_GetInputText(playerid, elementid, text[], len = sizeof(text));
native bool:DX_LoadFont(playerid, const fontFamily[], const url[]);
native bool:DX_DrawImage(playerid, elementid, const url[], Float:x, Float:y, Float:w, Float:h, color = 0xFFFFFFFF);
native bool:DX_DrawLine(playerid, elementid, Float:x1, Float:y1, Float:x2, Float:y2, Float:thickness, color);
native bool:DX_DrawCircle(playerid, elementid, Float:x, Float:y, Float:radius, color, Float:thickness = 0.0);
native bool:DX_SetClipArea(playerid, elementid, Float:x, Float:y, Float:w, Float:h);

native bool:DX_DrawGradientRectangle(playerid, elementid, Float:x, Float:y, Float:w, Float:h, colorTL, colorTR, colorBL, colorBR);
native bool:DX_DrawRoundedRectangle(playerid, elementid, Float:x, Float:y, Float:w, Float:h, Float:radius, color);
native bool:DX_DrawTriangle(playerid, elementid, Float:x1, Float:y1, Float:x2, Float:y2, Float:x3, Float:y3, color);

native bool:DX_DrawSlider(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:value, const font[] = "");
native bool:DX_DrawComboBox(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, selectedIndex, const options[], const font[] = "");
native bool:DX_DrawListView(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, selectedIndex, const items[], const font[] = "");
native bool:DX_DrawTabPanel(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, selectedIndex, const tabs[], const font[] = "");
native bool:DX_GetSliderValue(playerid, elementid, &Float:value);
native bool:DX_GetComboBoxIndex(playerid, elementid, &index);
native bool:DX_GetListViewIndex(playerid, elementid, &index);
native bool:DX_GetTabActive(playerid, elementid, &index);
native bool:DX_SetDraggable(playerid, elementid, bool:draggable);
native bool:DX_SetParent(playerid, elementid, parentid);
native bool:DX_DrawShadow(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, Float:size = 8.0, Float:offset = 4.0);
native bool:DX_SetTooltip(playerid, elementid, const tooltipText[]);
native bool:DX_DrawCircularProgress(playerid, elementid, Float:x, Float:y, Float:radius, Float:progress, color, Float:thickness);
native bool:DX_SetInputPassword(playerid, elementid, bool:enable);

native bool:DX_Animate(playerid, elementid, Float:targetX, Float:targetY, Float:targetW, Float:targetH, Float:targetAlpha, durationMs, easingType);
native bool:DX_DrawGraph(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, const Float:values[], numValues, Float:maxVal);
native bool:DX_DrawInventorySlot(playerid, elementid, Float:x, Float:y, Float:w, Float:h, color, const iconUrl[], const label[], amount);
native bool:DX_DrawTexturedProgressBar(playerid, elementid, Float:x, Float:y, Float:w, Float:h, const bgTextureUrl[], const fillTextureUrl[], Float:progress, color = 0xFFFFFFFF);
native bool:DX_DrawRadialMenu(playerid, elementid, Float:x, Float:y, Float:radius, color, selectedIndex, const items[], const icons[]);
native bool:DX_SetBlurBehind(playerid, elementid, bool:enable);
native bool:DX_DrawColorPicker(playerid, elementid, Float:x, Float:y, Float:w, Float:h, selectedColor);
native bool:DX_DrawScrollContainer(playerid, elementid, Float:x, Float:y, Float:w, Float:h, Float:contentHeight, Float:scrollVal, color);
native bool:DX_DrawIcon(playerid, elementid, Float:x, Float:y, Float:size, const iconName[], color, const font[] = "FontAwesome");
native bool:DX_PlaySound(playerid, const url[]);
native bool:DX_GetColorPickerColor(playerid, elementid, &color);
native bool:DX_GetScrollContainerVal(playerid, elementid, &Float:value);
native bool:DX_IsReady(playerid);

forward OnPlayerClickDX(playerid, elementid);
forward OnPlayerDXReady(playerid);
forward OnPlayerToggleDXCheckbox(playerid, elementid, bool:checked);
forward OnPlayerDXInputSubmit(playerid, elementid, const text[]);
forward OnPlayerChangeDXSlider(playerid, elementid, Float:value);
forward OnPlayerSelectDXComboBox(playerid, elementid, index);
forward OnPlayerSelectDXListView(playerid, elementid, index);
forward OnPlayerSelectDXTab(playerid, elementid, index);
forward OnPlayerDragDX(playerid, elementid, Float:x, Float:y);
forward OnPlayerSwapDXSlots(playerid, sourceElementid, targetElementid);
forward OnPlayerSelectRadialItem(playerid, elementid, index);
forward OnPlayerSelectDXColor(playerid, elementid, color);
forward OnPlayerScrollDXContainer(playerid, elementid, Float:scrollVal);


