#pragma once

enum LC_PROFILE_KEY
{
	// Settings.
	LC_PROFILE_FIXED_AXES,
	LC_PROFILE_LINE_WIDTH,
	LC_PROFILE_ALLOW_LOD,
	LC_PROFILE_LOD_DISTANCE,
	LC_PROFILE_FADE_STEPS,
	LC_PROFILE_FADE_STEPS_COLOR,
	LC_PROFILE_HIGHLIGHT_NEW_PARTS,
	LC_PROFILE_HIGHLIGHT_NEW_PARTS_COLOR,
	LC_PROFILE_SHADING_MODE,
	LC_PROFILE_BACKGROUND_GRADIENT,
	LC_PROFILE_BACKGROUND_COLOR,
	LC_PROFILE_GRADIENT_COLOR_TOP,
	LC_PROFILE_GRADIENT_COLOR_BOTTOM,
	LC_PROFILE_DRAW_AXES,
	LC_PROFILE_DRAW_AXES_LOCATION,
	LC_PROFILE_AXES_COLOR,
	LC_PROFILE_TEXT_COLOR,
	LC_PROFILE_MARQUEE_BORDER_COLOR,
	LC_PROFILE_MARQUEE_FILL_COLOR,
	LC_PROFILE_OVERLAY_COLOR,
	LC_PROFILE_ACTIVE_VIEW_COLOR,
	LC_PROFILE_INACTIVE_VIEW_COLOR,
	LC_PROFILE_DRAW_EDGE_LINES,
	LC_PROFILE_DRAW_CONDITIONAL_LINES,
	LC_PROFILE_GRID_STUDS,
	LC_PROFILE_GRID_STUD_COLOR,
	LC_PROFILE_GRID_LINES,
	LC_PROFILE_GRID_LINE_SPACING,
	LC_PROFILE_GRID_LINE_COLOR,
	LC_PROFILE_GRID_ORIGIN,
	LC_PROFILE_ANTIALIASING_SAMPLES,
	LC_PROFILE_VIEW_SPHERE_ENABLED,
	LC_PROFILE_VIEW_SPHERE_LOCATION,
	LC_PROFILE_VIEW_SPHERE_SIZE,
	LC_PROFILE_VIEW_SPHERE_COLOR,
	LC_PROFILE_VIEW_SPHERE_TEXT_COLOR,
	LC_PROFILE_VIEW_SPHERE_HIGHLIGHT_COLOR,
	LC_PROFILE_OBJECT_SELECTED_COLOR,
	LC_PROFILE_OBJECT_FOCUSED_COLOR,
	LC_PROFILE_CAMERA_COLOR,
	LC_PROFILE_LIGHT_COLOR,
	LC_PROFILE_CONTROL_POINT_COLOR,
	LC_PROFILE_CONTROL_POINT_FOCUSED_COLOR,

	LC_PROFILE_LANGUAGE,
	LC_PROFILE_COLOR_THEME,
	LC_PROFILE_CHECK_UPDATES,
	LC_PROFILE_PROJECTS_PATH,
	LC_PROFILE_PARTS_LIBRARY,
	LC_PROFILE_PART_PALETTES,
	LC_PROFILE_MINIFIG_SETTINGS,
	LC_PROFILE_COLOR_CONFIG,
	LC_PROFILE_KEYBOARD_SHORTCUTS,
	LC_PROFILE_MOUSE_SHORTCUTS,
	LC_PROFILE_CATEGORIES,
	LC_PROFILE_RECENT_FILE1,
	LC_PROFILE_RECENT_FILE2,
	LC_PROFILE_RECENT_FILE3,
	LC_PROFILE_RECENT_FILE4,
	LC_PROFILE_AUTOLOAD_MOSTRECENT,
	LC_PROFILE_RESTORE_TAB_LAYOUT,
	LC_PROFILE_AUTOSAVE_INTERVAL,
	LC_PROFILE_MOUSE_SENSITIVITY,
	LC_PROFILE_IMAGE_WIDTH,
	LC_PROFILE_IMAGE_HEIGHT,
	LC_PROFILE_IMAGE_EXTENSION,
	LC_PROFILE_PARTS_LIST_ICONS,
	LC_PROFILE_PARTS_LIST_NAMES,
	LC_PROFILE_PARTS_LIST_COLOR,
	LC_PROFILE_PARTS_LIST_DECORATED,
	LC_PROFILE_PARTS_LIST_ALIASES,
	LC_PROFILE_PARTS_LIST_LISTMODE,
	LC_PROFILE_STUD_STYLE,

	// Defaults for new projects.
	LC_PROFILE_DEFAULT_AUTHOR_NAME,
	LC_PROFILE_DEFAULT_AMBIENT_COLOR,

	// Exporters.
	LC_PROFILE_HTML_OPTIONS,
	LC_PROFILE_HTML_IMAGE_OPTIONS,
	LC_PROFILE_HTML_IMAGE_WIDTH,
	LC_PROFILE_HTML_IMAGE_HEIGHT,
	LC_PROFILE_POVRAY_PATH,
	LC_PROFILE_POVRAY_LGEO_PATH,
	LC_PROFILE_RENDER_WIDTH,
	LC_PROFILE_RENDER_HEIGHT,
	LC_PROFILE_BLENDER_PATH,
	LC_PROFILE_BLENDER_LDRAW_CONFIG_PATH,
	LC_PROFILE_BLENDER_VERSION,
	LC_PROFILE_BLENDER_ADDON_VERSION,
	LC_PROFILE_BLENDER_IMPORT_MODULE,

	LC_PROFILE_PREVIEW_VIEW_SPHERE_ENABLED,
	LC_PROFILE_PREVIEW_VIEW_SPHERE_SIZE,
	LC_PROFILE_PREVIEW_VIEW_SPHERE_LOCATION,
	LC_PROFILE_PREVIEW_DRAW_AXES,

	LC_PROFILE_STUD_CYLINDER_COLOR_ENABLED,
	LC_PROFILE_STUD_CYLINDER_COLOR,
	LC_PROFILE_PART_EDGE_COLOR_ENABLED,
	LC_PROFILE_PART_EDGE_COLOR,
	LC_PROFILE_BLACK_EDGE_COLOR_ENABLED,
	LC_PROFILE_BLACK_EDGE_COLOR,
	LC_PROFILE_DARK_EDGE_COLOR_ENABLED,
	LC_PROFILE_DARK_EDGE_COLOR,
	LC_PROFILE_PART_EDGE_CONTRAST,
	LC_PROFILE_PART_COLOR_VALUE_LD_INDEX,
	LC_PROFILE_AUTOMATE_EDGE_COLOR,

	LC_NUM_PROFILE_KEYS
};

enum LC_PROFILE_ENTRY_TYPE
{
	LC_PROFILE_ENTRY_INT,
	LC_PROFILE_ENTRY_FLOAT,
	LC_PROFILE_ENTRY_STRING,
	LC_PROFILE_ENTRY_STRINGLIST,
	LC_PROFILE_ENTRY_BUFFER
};

class lcProfileEntry
{
public:
	lcProfileEntry(const char* Section, const char* Key, int DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, uint DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, float DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, const char* DefaultValue);
	lcProfileEntry(const char* Section, const char* Key, const QStringList& StringList);
	lcProfileEntry(const char* Section, const char* Key);

	LC_PROFILE_ENTRY_TYPE mType;

	const char* mSection;
	const char* mKey;

	union
	{
		int IntValue;
		uint UIntValue;
		float FloatValue;
		const char* StringValue;
	} mDefault;
};

void lcRemoveProfileKey(LC_PROFILE_KEY Key);

int lcGetProfileInt(LC_PROFILE_KEY Key);
uint lcGetProfileUInt(LC_PROFILE_KEY Key);
float lcGetProfileFloat(LC_PROFILE_KEY Key);
QString lcGetProfileString(LC_PROFILE_KEY Key);
QStringList lcGetProfileStringList(LC_PROFILE_KEY Key);
QByteArray lcGetProfileBuffer(LC_PROFILE_KEY Key);

void lcSetProfileInt(LC_PROFILE_KEY Key, int Value);
void lcSetProfileUInt(LC_PROFILE_KEY Key, uint Value);
void lcSetProfileFloat(LC_PROFILE_KEY Key, float Value);
void lcSetProfileString(LC_PROFILE_KEY Key, const QString& Value);
void lcSetProfileStringList(LC_PROFILE_KEY Key, const QStringList& Value);
void lcSetProfileBuffer(LC_PROFILE_KEY Key, const QByteArray& Buffer);
