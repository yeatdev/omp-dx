#if defined _INC_test_defs
	#endinput
#endif
#define _INC_test_defs

// ==================== DEFINITIONS & ID ALLOCATIONS ====================

// Speedometer HUD (Bottom Right Corner)
#define UI_SPEEDOMETER_BG       200
#define UI_SPEED_DIAL           201
#define UI_SPEED_NEEDLE         202
#define UI_SPEED_TEXT           203
#define UI_SPEED_UNIT           204
#define UI_FUEL_BG              205
#define UI_FUEL_VAL             206
#define UI_FUEL_LABEL           207
#define UI_ENGINE_BTN           208
#define UI_LIGHTS_BTN           209
#define UI_LOCK_BTN             210
#define UI_SEATBELT_IMG         211
#define UI_SEATBELT_LABEL       212

// Vehicle Diagnostics & GPS Dashboard (Center Screen)
#define UI_DASH_BG              300
#define UI_DASH_HEADER          301
#define UI_DASH_TITLE           302
#define UI_DASH_MAP_IMG         303
#define UI_DASH_MAP_RING        304
#define UI_DASH_COMPASS_N       305
#define UI_DASH_COMPASS_S       306
#define UI_DASH_LIMITER_INPUT   307
#define UI_DASH_LIMITER_LABEL   308
#define UI_DASH_TEMP_LABEL      309
#define UI_DASH_TEMP_BG         310
#define UI_DASH_TEMP_VAL        311
#define UI_DASH_SAVE            312
#define UI_DASH_CLOSE           313
#define UI_DASH_SIDEBAR         314
#define UI_DASH_CLIP_START      320
#define UI_DASH_LOG_1_BG        321
#define UI_DASH_LOG_1_TXT       322
#define UI_DASH_LOG_2_BG        323
#define UI_DASH_LOG_2_TXT       324
#define UI_DASH_LOG_3_BG        325
#define UI_DASH_LOG_3_TXT       326
#define UI_DASH_LOG_4_BG        327
#define UI_DASH_LOG_4_TXT       328
#define UI_DASH_CLIP_END        329

// Widget Test Panel
#define UI_WIDGET_BG            500
#define UI_WIDGET_HEADER        501
#define UI_WIDGET_TITLE         502
#define UI_WIDGET_SLIDER_LBL    503
#define UI_WIDGET_SLIDER        504
#define UI_WIDGET_SLIDER_VAL    505
#define UI_WIDGET_COMBO_LBL     506
#define UI_WIDGET_COMBO         507
#define UI_WIDGET_COMBO_VAL     508
#define UI_WIDGET_LIST_LBL      509
#define UI_WIDGET_LIST          510
#define UI_WIDGET_TAB_LBL       511
#define UI_WIDGET_TAB           512
#define UI_WIDGET_TAB_CONTENT   513
#define UI_WIDGET_CLOSE         514
#define UI_WIDGET_SIDEBAR       515
#define UI_WIDGET_PASS_LBL      516
#define UI_WIDGET_PASS_INPUT    517
#define UI_WIDGET_CIRC_LBL      518
#define UI_WIDGET_CIRC_PROGRESS 519
#define UI_WIDGET_SHADOW        499

// New Test Panel
#define UI_NEW_BG            600
#define UI_NEW_HEADER        601
#define UI_NEW_TITLE         602
#define UI_NEW_CLOSE         603
#define UI_NEW_PASS_LBL      604
#define UI_NEW_PASS_INPUT    605
#define UI_NEW_SLIDER_LBL    606
#define UI_NEW_SLIDER        607
#define UI_NEW_SLIDER_VAL    608
#define UI_NEW_CIRC_LBL      609
#define UI_NEW_CIRC_PROGRESS 610
#define UI_NEW_SHADOW        599
#define UI_NEW_COLOR_LBL     611
#define UI_NEW_COLOR_PICKER  612
#define UI_NEW_COLOR_PREVIEW 613
#define UI_NEW_SCROLL_LBL    614
#define UI_NEW_SCROLL_CONT   615
#define UI_NEW_SCROLL_ITEM1  616
#define UI_NEW_SCROLL_ITEM2  617
#define UI_NEW_SCROLL_ITEM3  618
#define UI_NEW_ICON_LBL      619
#define UI_NEW_PLAY_SOUND    620
#define UI_NEW_GRAPH_LBL     621
#define UI_NEW_GRAPH         622
#define UI_NEW_TEX_BAR_LBL   623
#define UI_NEW_TEX_BAR       624
#define UI_NEW_ICON_HEART    625
#define UI_NEW_ICON_STAR     626
#define UI_NEW_ICON_MUSIC    627
#define UI_NEW_ICON_BELL     628

// ==================== STATE VARIABLES ====================
new SpeedometerTimer[MAX_PLAYERS] = {-1, ...};
new Float:WidgetPanelX[MAX_PLAYERS];
new Float:WidgetPanelY[MAX_PLAYERS];
new Float:NewPanelX[MAX_PLAYERS];
new Float:NewPanelY[MAX_PLAYERS];

new bool:VehicleEngine[MAX_PLAYERS] = {false, ...};
new bool:VehicleLights[MAX_PLAYERS] = {false, ...};
new bool:VehicleLock[MAX_PLAYERS] = {false, ...};
new bool:VehicleSeatbelt[MAX_PLAYERS] = {false, ...};
new Float:VehicleFuel[MAX_PLAYERS] = {85.0, ...};
new Float:EngineTemp[MAX_PLAYERS] = {90.0, ...};
new SpeedLimiterVal[MAX_PLAYERS] = {120, ...};

// ==================== STATE HELPERS ====================

stock DX_GetLimiterTextLength(playerid)
{
	new testStr[64];
	if (DX_GetInputText(playerid, UI_DASH_LIMITER_INPUT, testStr))
	{
		return 1;
	}
	return 0;
}

stock bool:IsDashboardOpen(playerid)
{
	return (DX_GetLimiterTextLength(playerid) > 0); 
}
