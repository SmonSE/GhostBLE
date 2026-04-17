#pragma once

// ===== Screen Dimensions =====
// Cardputer, M5StickCPlus2 & M5StickS3: 240x135 (landscape rotation)
#define SCREEN_W 240
#define SCREEN_H 135

// ===== UI Layout Constants =====
// NibBLEs sprite positions
#define NIBBLES_FRONT_X        5
#define NIBBLES_FRONT_Y        0
#define NIBBLES_HAPPY_X        83
#define NIBBLES_HAPPY_Y        60
#define NIBBLES_SLEEP_X        83
#define NIBBLES_SLEEP_Y        60

// Speech/thought bubble geometry
#define BUBBLE_X               125
#define BUBBLE_MAX_W           108

#define BUBBLE_RECT_Y          15
#define BUBBLE_RECT_W          108
#define BUBBLE_RECT_H          22
#define BUBBLE_CORNER_R        4
#define BUBBLE_TRI_OFFSET_X    8
#define BUBBLE_TRI_W           6
#define BUBBLE_TRI_H           5
#define BUBBLE_TEXT_INSET_X    6
#define BUBBLE_TEXT_INSET_Y    7
#define BUBBLE_MIN_W           60
#define THOUGHT_BUBBLE_Y       18

// Font metrics (default font, text size 1)
#define CHAR_WIDTH_PX          6
#define BUBBLE_PADDING_PX      16

// Top status bar
#define STATUS_BAR_Y           2
#define STATUS_ICON_X          5

// Stats panel (left side)
#define STATS_X                5
#define STATS_Y_START          67
#define STATS_LINE_HEIGHT      12

// Bottom bar
#define BOTTOM_BAR_Y           127
#define XP_BAR_W               70

#define LEVEL_TEXT_X           5
#define XP_BAR_X               38
#define XP_BAR_H               7
#define TITLE_TEXT_X           112

// Battery icon position
#define BATTERY_ICON_X         215

// Border/outline color for white speech bubbles
#define BUBBLE_BORDER_COLOR    0x2104

// ===== Button Long Press Threshold =====
#define LONG_PRESS_MS          1000
#define HELP_LONG_PRESS_MS     3000
