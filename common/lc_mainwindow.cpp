#include "lc_global.h"
#include "lc_mainwindow.h"
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include "lc_partselectionwidget.h"
#include "lc_timelinewidget.h"
#include "lc_viewwidget.h"
#include "lc_colorlist.h"
#include "lc_qpropertiestree.h"
#include "lc_qutils.h"
#include "lc_qupdatedialog.h"
#include "lc_aboutdialog.h"
#include "lc_setsdatabasedialog.h"
#include "lc_qhtmldialog.h"
#include "lc_renderdialog.h"
#include "lc_instructionsdialog.h"
#include "lc_profile.h"
#include "lc_view.h"
#include "project.h"
#include "piece.h"
#include "camera.h"
#include "group.h"
#include "pieceinf.h"
#include "lc_library.h"
#include "lc_colors.h"
#include "lc_previewwidget.h"

#if LC_ENABLE_GAMEPAD
#include <QtGamepad/QGamepad>
#endif

lcMainWindow* gMainWindow;
#define LC_TAB_LAYOUT_VERSION 0x0001

void lcTabBar::mousePressEvent(QMouseEvent* Event)
{
	if (Event->button() == Qt::MiddleButton)
		mMousePressTab = tabAt(Event->pos());
	else
		QTabBar::mousePressEvent(Event);
}

void lcTabBar::mouseReleaseEvent(QMouseEvent* Event)
{
	if (Event->button() == Qt::MiddleButton && tabAt(Event->pos()) == mMousePressTab)
		tabCloseRequested(mMousePressTab);
	else
		QTabBar::mouseReleaseEvent(Event);
}

lcMainWindow::lcMainWindow()
{
	memset(mActions, 0, sizeof(mActions));

	mTransformType = lcTransformType::RelativeTranslation;

	mColorIndex = lcGetColorIndex(7);
	mTool = lcTool::Select;
	mAddKeys = false;
	mMoveSnapEnabled = true;
	mAngleSnapEnabled = true;
	mMoveXYSnapIndex = 4;
	mMoveZSnapIndex = 3;
	mAngleSnapIndex = 5;
	mRelativeTransform = true;
	mLocalTransform = false;
	mCurrentPieceInfo = nullptr;
	mSelectionMode = lcSelectionMode::Single;
	mModelTabWidget = nullptr;
	mPreviewToolBar = nullptr;
	mPreviewWidget = nullptr;

	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		mRecentFiles[FileIdx] = lcGetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
	connect(&mGamepadTimer, &QTimer::timeout, this, &lcMainWindow::UpdateGamepads);
	mLastGamepadUpdate = QDateTime::currentDateTime();
	mGamepadTimer.start(33);
#endif

	gMainWindow = this;
}

lcMainWindow::~lcMainWindow()
{
	if (mCurrentPieceInfo)
	{
		lcPiecesLibrary* Library = lcGetPiecesLibrary();
		Library->ReleasePieceInfo(mCurrentPieceInfo);
		mCurrentPieceInfo = nullptr;
	}

	for (int FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		lcSetProfileString((LC_PROFILE_KEY)(LC_PROFILE_RECENT_FILE1 + FileIdx), mRecentFiles[FileIdx]);

	gMainWindow = nullptr;
}

void lcMainWindow::CreateWidgets()
{
	setAcceptDrops(true);
	setWindowIcon(QIcon(":/resources/leocad.png"));
	setWindowFilePath(QString());

	CreateActions();
	CreateToolBars();
	CreateMenus();
	CreateStatusBar();

	mModelTabWidget = new QTabWidget();
	mModelTabWidget->tabBar()->setMovable(true);
	mModelTabWidget->tabBar()->setTabsClosable(true);
	mModelTabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
	setCentralWidget(mModelTabWidget);

	connect(mModelTabWidget->tabBar(), SIGNAL(tabCloseRequested(int)), this, SLOT(ModelTabClosed(int)));
	connect(mModelTabWidget->tabBar(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ModelTabContextMenuRequested(const QPoint&)));
	connect(mModelTabWidget, SIGNAL(currentChanged(int)), this, SLOT(ModelTabChanged(int)));

	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(ClipboardChanged()));
	ClipboardChanged();

	QSettings Settings;
	Settings.beginGroup("MainWindow");
	resize(QSize(800, 600));
	move(QPoint(200, 200));
	restoreGeometry(Settings.value("Geometry").toByteArray());
	restoreState(Settings.value("State").toByteArray());
	mPartSelectionWidget->LoadState(Settings);
	Settings.endGroup();
}

void lcMainWindow::CreateActions()
{
	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		QAction* Action = new QAction(qApp->translate("Menu", gCommands[CommandIdx].MenuName), this);
		Action->setStatusTip(qApp->translate("Status", gCommands[CommandIdx].StatusText));
		connect(Action, SIGNAL(triggered()), this, SLOT(ActionTriggered()));
		addAction(Action);
		mActions[CommandIdx] = Action;
	}

	mActions[LC_FILE_NEW]->setToolTip(tr("New Model"));
	mActions[LC_FILE_OPEN]->setToolTip(tr("Open Model"));
	mActions[LC_FILE_SAVE]->setToolTip(tr("Save Model"));

	QIcon FileNewIcon;
	FileNewIcon.addFile(":/resources/file_new.png");
	FileNewIcon.addFile(":/resources/file_new_16.png");
	mActions[LC_FILE_NEW]->setIcon(FileNewIcon);

	QIcon FileSaveIcon;
	FileSaveIcon.addFile(":/resources/file_save.png");
	FileSaveIcon.addFile(":/resources/file_save_16.png");
	mActions[LC_FILE_SAVE]->setIcon(FileSaveIcon);

	QIcon FileOpenIcon;
	FileOpenIcon.addFile(":/resources/file_open.png");
	FileOpenIcon.addFile(":/resources/file_open_16.png");
	mActions[LC_FILE_OPEN]->setIcon(FileOpenIcon);

	QIcon FilePrintIcon;
	FilePrintIcon.addFile(":/resources/file_print.png");
	FilePrintIcon.addFile(":/resources/file_print_16.png");
	mActions[LC_FILE_PRINT]->setIcon(FilePrintIcon);

	QIcon FilePrintPreviewIcon;
	FilePrintPreviewIcon.addFile(":/resources/file_print_preview.png");
	FilePrintPreviewIcon.addFile(":/resources/file_print_preview_16.png");
	mActions[LC_FILE_PRINT_PREVIEW]->setIcon(FilePrintPreviewIcon);

	QIcon EditUndoIcon;
	EditUndoIcon.addFile(":/resources/edit_undo.png");
	EditUndoIcon.addFile(":/resources/edit_undo_16.png");
	mActions[LC_EDIT_UNDO]->setIcon(EditUndoIcon);

	QIcon EditRedoIcon;
	EditRedoIcon.addFile(":/resources/edit_redo.png");
	EditRedoIcon.addFile(":/resources/edit_redo_16.png");
	mActions[LC_EDIT_REDO]->setIcon(EditRedoIcon);

	QIcon EditCutIcon;
	EditCutIcon.addFile(":/resources/edit_cut.png");
	EditCutIcon.addFile(":/resources/edit_cut_16.png");
	mActions[LC_EDIT_CUT]->setIcon(EditCutIcon);

	QIcon EditCopyIcon;
	EditCopyIcon.addFile(":/resources/edit_copy.png");
	EditCopyIcon.addFile(":/resources/edit_copy_16.png");
	mActions[LC_EDIT_COPY]->setIcon(EditCopyIcon);

	QIcon EditPasteIcon;
	EditPasteIcon.addFile(":/resources/edit_paste.png");
	EditPasteIcon.addFile(":/resources/edit_paste_16.png");
	mActions[LC_EDIT_PASTE]->setIcon(EditPasteIcon);

	QIcon EditActionInsertIcon;
	EditActionInsertIcon.addFile(":/resources/action_insert.png");
	EditActionInsertIcon.addFile(":/resources/action_insert_16.png");
	mActions[LC_EDIT_ACTION_INSERT]->setIcon(EditActionInsertIcon);

	QIcon EditActionLightIcon;
	EditActionLightIcon.addFile(":/resources/action_light.png");
	EditActionLightIcon.addFile(":/resources/action_light_16.png");
	mActions[LC_EDIT_ACTION_LIGHT]->setIcon(EditActionLightIcon);

	QIcon EditActionSpotLightIcon;
	EditActionSpotLightIcon.addFile(":/resources/action_spotlight.png");
	EditActionSpotLightIcon.addFile(":/resources/action_spotlight_16.png");
	mActions[LC_EDIT_ACTION_SPOTLIGHT]->setIcon(EditActionSpotLightIcon);

	QIcon EditActionSelectIcon;
	EditActionSelectIcon.addFile(":/resources/action_select.png");
	EditActionSelectIcon.addFile(":/resources/action_select_16.png");
	mActions[LC_EDIT_ACTION_SELECT]->setIcon(EditActionSelectIcon);

	QIcon EditActionMoveIcon;
	EditActionMoveIcon.addFile(":/resources/action_move.png");
	EditActionMoveIcon.addFile(":/resources/action_move_16.png");
	mActions[LC_EDIT_ACTION_MOVE]->setIcon(EditActionMoveIcon);

	QIcon EditActionRotateIcon;
	EditActionRotateIcon.addFile(":/resources/action_rotate.png");
	EditActionRotateIcon.addFile(":/resources/action_rotate_16.png");
	mActions[LC_EDIT_ACTION_ROTATE]->setIcon(EditActionRotateIcon);

	QIcon EditActionDeleteIcon;
	EditActionDeleteIcon.addFile(":/resources/action_delete.png");
	EditActionDeleteIcon.addFile(":/resources/action_delete_16.png");
	mActions[LC_EDIT_ACTION_DELETE]->setIcon(EditActionDeleteIcon);

	QIcon EditActionPaintIcon;
	EditActionPaintIcon.addFile(":/resources/action_paint.png");
	EditActionPaintIcon.addFile(":/resources/action_paint_16.png");
	mActions[LC_EDIT_ACTION_PAINT]->setIcon(EditActionPaintIcon);

	QIcon EditActionColorPickerIcon;
	EditActionColorPickerIcon.addFile(":/resources/action_color_picker.png");
	EditActionColorPickerIcon.addFile(":/resources/action_color_picker_16.png");
	mActions[LC_EDIT_ACTION_COLOR_PICKER]->setIcon(EditActionColorPickerIcon);

	QIcon EditActionZoomIcon;
	EditActionZoomIcon.addFile(":/resources/action_zoom.png");
	EditActionZoomIcon.addFile(":/resources/action_zoom_16.png");
	mActions[LC_EDIT_ACTION_ZOOM]->setIcon(EditActionZoomIcon);

	QIcon EditActionPanIcon;
	EditActionPanIcon.addFile(":/resources/action_pan.png");
	EditActionPanIcon.addFile(":/resources/action_pan_16.png");
	mActions[LC_EDIT_ACTION_PAN]->setIcon(EditActionPanIcon);

	QIcon FileRenderPOVRayIcon;
	FileRenderPOVRayIcon.addFile(":/resources/file_render_povray.png");
	FileRenderPOVRayIcon.addFile(":/resources/file_render_povray_16.png");
	mActions[LC_FILE_RENDER_POVRAY]->setIcon(FileRenderPOVRayIcon);

	QIcon FileRenderBlenderIcon;
	FileRenderBlenderIcon.addFile(":/resources/file_render_blender.png");
	FileRenderBlenderIcon.addFile(":/resources/file_render_blender_16.png");
	mActions[LC_FILE_RENDER_BLENDER]->setIcon(FileRenderBlenderIcon);

	QIcon FileOpenInBlenderIcon;
	FileOpenInBlenderIcon.addFile(":/resources/file_render_open_in_blender.png");
	FileOpenInBlenderIcon.addFile(":/resources/file_render_open_in_blender_16.png");
	mActions[LC_FILE_RENDER_OPEN_IN_BLENDER]->setIcon(FileOpenInBlenderIcon);

	mActions[LC_EDIT_ACTION_CAMERA]->setIcon(QIcon(":/resources/action_camera.png"));
	mActions[LC_EDIT_ACTION_ROTATE_VIEW]->setIcon(QIcon(":/resources/action_rotate_view.png"));
	mActions[LC_EDIT_ACTION_ROLL]->setIcon(QIcon(":/resources/action_roll.png"));
	mActions[LC_EDIT_ACTION_ZOOM_REGION]->setIcon(QIcon(":/resources/action_zoom_region.png"));
	mActions[LC_EDIT_FIND]->setIcon(QIcon(":/resources/edit_find.png"));
	mActions[LC_EDIT_FIND_NEXT]->setIcon(QIcon(":/resources/edit_find_next.png"));
	mActions[LC_EDIT_FIND_PREVIOUS]->setIcon(QIcon(":/resources/edit_find_previous.png"));
	mActions[LC_EDIT_FIND_ALL]->setIcon(QIcon(":/resources/edit_find_all.png"));
	mActions[LC_EDIT_REPLACE_NEXT]->setIcon(QIcon(":/resources/edit_replace_next.png"));
	mActions[LC_EDIT_REPLACE_ALL]->setIcon(QIcon(":/resources/edit_replace_all.png"));
	mActions[LC_PIECE_SHOW_EARLIER]->setIcon(QIcon(":/resources/piece_show_earlier.png"));
	mActions[LC_PIECE_SHOW_LATER]->setIcon(QIcon(":/resources/piece_show_later.png"));
	mActions[LC_VIEW_SPLIT_HORIZONTAL]->setIcon(QIcon(":/resources/view_split_horizontal.png"));
	mActions[LC_VIEW_SPLIT_VERTICAL]->setIcon(QIcon(":/resources/view_split_vertical.png"));
	mActions[LC_VIEW_ZOOM_IN]->setIcon(QIcon(":/resources/view_zoomin.png"));
	mActions[LC_VIEW_ZOOM_OUT]->setIcon(QIcon(":/resources/view_zoomout.png"));
	mActions[LC_VIEW_ZOOM_EXTENTS]->setIcon(QIcon(":/resources/view_zoomextents.png"));
	mActions[LC_VIEW_TIME_FIRST]->setIcon(QIcon(":/resources/time_first.png"));
	mActions[LC_VIEW_TIME_PREVIOUS]->setIcon(QIcon(":/resources/time_previous.png"));
	mActions[LC_VIEW_TIME_NEXT]->setIcon(QIcon(":/resources/time_next.png"));
	mActions[LC_VIEW_TIME_LAST]->setIcon(QIcon(":/resources/time_last.png"));
	mActions[LC_VIEW_TIME_ADD_KEYS]->setIcon(QIcon(":/resources/time_add_keys.png"));
	mActions[LC_HELP_HOMEPAGE]->setIcon(QIcon(":/resources/help_homepage.png"));

	mActions[LC_EDIT_SNAP_MOVE_TOGGLE]->setCheckable(true);
	mActions[LC_EDIT_SNAP_ANGLE_TOGGLE]->setCheckable(true);
	mActions[LC_VIEW_CAMERA_NONE]->setCheckable(true);
	mActions[LC_VIEW_TIME_ADD_KEYS]->setCheckable(true);

	for (int ActionIndex = LC_VIEW_TOOLBAR_FIRST; ActionIndex <= LC_VIEW_TOOLBAR_LAST; ActionIndex++)
		mActions[ActionIndex]->setCheckable(true);

	QActionGroup* ActionRelativeGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_TRANSFORM_RELATIVE; ActionIdx <= LC_EDIT_TRANSFORM_ABSOLUTE; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionRelativeGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionSeparateGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_TRANSFORM_SEPARATELY; ActionIdx <= LC_EDIT_TRANSFORM_TOGETHER; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSeparateGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionSnapXYGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SNAP_MOVE_XY0; ActionIdx <= LC_EDIT_SNAP_MOVE_XY9; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSnapXYGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionSnapZGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SNAP_MOVE_Z0; ActionIdx <= LC_EDIT_SNAP_MOVE_Z9; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSnapZGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionSnapAngleGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SNAP_ANGLE0; ActionIdx <= LC_EDIT_SNAP_ANGLE9; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionSnapAngleGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionTransformTypeGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION; ActionIdx <= LC_EDIT_TRANSFORM_RELATIVE_ROTATION; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionTransformTypeGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionToolGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_ACTION_FIRST; ActionIdx <= LC_EDIT_ACTION_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionToolGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionCameraGroup = new QActionGroup(this);
	ActionCameraGroup->addAction(mActions[LC_VIEW_CAMERA_NONE]);
	for (int ActionIdx = LC_VIEW_CAMERA_FIRST; ActionIdx <= LC_VIEW_CAMERA_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionCameraGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionPerspectiveGroup = new QActionGroup(this);
	for (int ActionIdx = LC_VIEW_PROJECTION_FIRST; ActionIdx <= LC_VIEW_PROJECTION_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionPerspectiveGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ActionShadingGroup = new QActionGroup(this);
	for (int ActionIdx = LC_VIEW_SHADING_FIRST; ActionIdx <= LC_VIEW_SHADING_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ActionShadingGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* SelectionModeGroup = new QActionGroup(this);
	for (int ActionIdx = LC_EDIT_SELECTION_MODE_FIRST; ActionIdx <= LC_EDIT_SELECTION_MODE_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		SelectionModeGroup->addAction(mActions[ActionIdx]);
	}

	QActionGroup* ModelGroup = new QActionGroup(this);
	for (int ActionIdx = LC_MODEL_FIRST; ActionIdx <= LC_MODEL_LAST; ActionIdx++)
	{
		mActions[ActionIdx]->setCheckable(true);
		ModelGroup->addAction(mActions[ActionIdx]);
	}

	UpdateShortcuts();
}

void lcMainWindow::CreateMenus()
{
    QMenu* TransformMenu = new QMenu(tr("Transform and Rotat&e"), this);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION]);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION]);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_RELATIVE_ROTATION]);
	TransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION]);
	mActions[LC_EDIT_TRANSFORM]->setMenu(TransformMenu);

	mCameraMenu = new QMenu(tr("C&ameras"), this);
	mCameraMenu->addAction(mActions[LC_VIEW_CAMERA_NONE]);

	for (int actionIdx = LC_VIEW_CAMERA_FIRST; actionIdx <= LC_VIEW_CAMERA_LAST; actionIdx++)
		mCameraMenu->addAction(mActions[actionIdx]);

	mViewpointMenu = new QMenu(tr("&Viewpoints"), this);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_FRONT]);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_BACK]);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_LEFT]);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_RIGHT]);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_TOP]);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_BOTTOM]);
	mViewpointMenu->addAction(mActions[LC_VIEW_VIEWPOINT_HOME]);

    mProjectionMenu = new QMenu(tr("Pro&jection"), this);
	mProjectionMenu->addAction(mActions[LC_VIEW_PROJECTION_PERSPECTIVE]);
	mProjectionMenu->addAction(mActions[LC_VIEW_PROJECTION_ORTHO]);

	mShadingMenu = new QMenu(tr("Sh&ading"), this);
	mShadingMenu->addAction(mActions[LC_VIEW_SHADING_WIREFRAME]);
	mShadingMenu->addAction(mActions[LC_VIEW_SHADING_FLAT]);
	mShadingMenu->addAction(mActions[LC_VIEW_SHADING_DEFAULT_LIGHTS]);

    mToolsMenu = new QMenu(tr("T&ools"), this);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_INSERT]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_LIGHT]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_SPOTLIGHT]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_CAMERA]);
	mToolsMenu->addSeparator();
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_SELECT]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_MOVE]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_ROTATE]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_DELETE]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_PAINT]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_COLOR_PICKER]);
	mToolsMenu->addSeparator();
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_ZOOM]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_PAN]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_ROTATE_VIEW]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_ROLL]);
	mToolsMenu->addAction(mActions[LC_EDIT_ACTION_ZOOM_REGION]);

	QMenu* FileMenu = menuBar()->addMenu(tr("&File"));
	FileMenu->addAction(mActions[LC_FILE_NEW]);
	FileMenu->addAction(mActions[LC_FILE_OPEN]);
	FileMenu->addAction(mActions[LC_FILE_MERGE]);
	FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_SAVE]);
	FileMenu->addAction(mActions[LC_FILE_SAVEAS]);
	FileMenu->addAction(mActions[LC_FILE_SAVE_IMAGE]);
	QMenu* ImportMenu = FileMenu->addMenu(tr("&Import"));
	ImportMenu->addAction(mActions[LC_FILE_IMPORT_LDD]);
	ImportMenu->addAction(mActions[LC_FILE_IMPORT_INVENTORY]);
	QMenu* ExportMenu = FileMenu->addMenu(tr("&Export"));
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_3DS]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_BRICKLINK]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_COLLADA]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_CSV]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_HTML]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_POVRAY]);
	ExportMenu->addAction(mActions[LC_FILE_EXPORT_WAVEFRONT]);
	FileMenu->addSeparator();
	QMenu* RenderMenu = FileMenu->addMenu(tr("&Render"));
	RenderMenu->addAction(mActions[LC_FILE_RENDER_POVRAY]);
	RenderMenu->addAction(mActions[LC_FILE_RENDER_BLENDER]);
	RenderMenu->addAction(mActions[LC_FILE_RENDER_OPEN_IN_BLENDER]);
	FileMenu->addAction(mActions[LC_FILE_INSTRUCTIONS]);
	FileMenu->addAction(mActions[LC_FILE_PRINT]);
	FileMenu->addAction(mActions[LC_FILE_PRINT_PREVIEW]);
	FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_RECENT1]);
	FileMenu->addAction(mActions[LC_FILE_RECENT2]);
	FileMenu->addAction(mActions[LC_FILE_RECENT3]);
	FileMenu->addAction(mActions[LC_FILE_RECENT4]);
	mActionFileRecentSeparator = FileMenu->addSeparator();
	FileMenu->addAction(mActions[LC_FILE_EXIT]);

	QMenu* EditMenu = menuBar()->addMenu(tr("&Edit"));
	EditMenu->addAction(mActions[LC_EDIT_UNDO]);
	EditMenu->addAction(mActions[LC_EDIT_REDO]);
	EditMenu->addSeparator();
	EditMenu->addAction(mActions[LC_EDIT_CUT]);
	EditMenu->addAction(mActions[LC_EDIT_COPY]);
	EditMenu->addAction(mActions[LC_EDIT_PASTE]);
	EditMenu->addAction(mActions[LC_EDIT_PASTE_STEPS]);
	EditMenu->addSeparator();
	EditMenu->addAction(mActions[LC_EDIT_FIND]);
	EditMenu->addAction(mActions[LC_EDIT_FIND_NEXT]);
	EditMenu->addAction(mActions[LC_EDIT_FIND_PREVIOUS]);
	EditMenu->addAction(mActions[LC_EDIT_REPLACE]);
	EditMenu->addAction(mActions[LC_EDIT_REPLACE_NEXT]);
	EditMenu->addSeparator();
	EditMenu->addAction(mActions[LC_EDIT_SELECT_ALL]);
	EditMenu->addAction(mActions[LC_EDIT_SELECT_NONE]);
	EditMenu->addAction(mActions[LC_EDIT_SELECT_INVERT]);
	EditMenu->addAction(mActions[LC_EDIT_SELECT_BY_NAME]);
	EditMenu->addMenu(mSelectionModeMenu);
	EditMenu->addSeparator();
	EditMenu->addMenu(mTransformMenu);
	EditMenu->addMenu(mToolsMenu);

	QMenu* ViewMenu = menuBar()->addMenu(tr("&View"));
	ViewMenu->addAction(mActions[LC_VIEW_PREFERENCES]);
	ViewMenu->addSeparator();
	ViewMenu->addAction(mActions[LC_VIEW_ZOOM_EXTENTS]);
	ViewMenu->addAction(mActions[LC_VIEW_LOOK_AT]);
	ViewMenu->addMenu(mViewpointMenu);
	ViewMenu->addMenu(mCameraMenu);
	ViewMenu->addMenu(mProjectionMenu);
	ViewMenu->addMenu(mShadingMenu);
	QMenu* StepMenu = ViewMenu->addMenu(tr("Ste&p"));
	StepMenu->addAction(mActions[LC_VIEW_TIME_FIRST]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_PREVIOUS]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_NEXT]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_LAST]);
	StepMenu->addSeparator();
	StepMenu->addAction(mActions[LC_VIEW_TIME_INSERT_BEFORE]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_INSERT_AFTER]);
	StepMenu->addAction(mActions[LC_VIEW_TIME_DELETE]);
	ViewMenu->addSeparator();
	ViewMenu->addAction(mActions[LC_VIEW_SPLIT_HORIZONTAL]);
	ViewMenu->addAction(mActions[LC_VIEW_SPLIT_VERTICAL]);
	ViewMenu->addAction(mActions[LC_VIEW_REMOVE_VIEW]);
	ViewMenu->addAction(mActions[LC_VIEW_RESET_VIEWS]);
	ViewMenu->addSeparator();
	QMenu* ToolBarsMenu = ViewMenu->addMenu(tr("T&oolbars"));
	connect(ToolBarsMenu, SIGNAL(aboutToShow()), this, SLOT(UpdateDockWidgetActions()));
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_PARTS]);
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_COLORS]);
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_PROPERTIES]);
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_TIMELINE]);
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_PREVIEW]);
	ToolBarsMenu->addSeparator();
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_STANDARD]);
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_TOOLS]);
	ToolBarsMenu->addAction(mActions[LC_VIEW_TOOLBAR_TIME]);
	ViewMenu->addAction(mActions[LC_VIEW_FULLSCREEN]);

	QMenu* PieceMenu = menuBar()->addMenu(tr("&Piece"));
	PieceMenu->addAction(mActions[LC_PIECE_INSERT]);
	PieceMenu->addAction(mActions[LC_PIECE_DELETE]);
    PieceMenu->addAction(mActions[LC_PIECE_DUPLICATE]);
    PieceMenu->addAction(mActions[LC_PIECE_REPLACE]);
    PieceMenu->addAction(mActions[LC_PIECE_PAINT_SELECTED]);
	PieceMenu->addAction(mActions[LC_PIECE_ARRAY]);
	PieceMenu->addAction(mActions[LC_PIECE_MINIFIG_WIZARD]);
	PieceMenu->addAction(mActions[LC_PIECE_RESET_PIVOT_POINT]);
	PieceMenu->addAction(mActions[LC_PIECE_REMOVE_KEY_FRAMES]);
	PieceMenu->addSeparator();
	PieceMenu->addAction(mActions[LC_PIECE_EDIT_SELECTED_SUBMODEL]);
	PieceMenu->addAction(mActions[LC_PIECE_EDIT_END_SUBMODEL]);
	PieceMenu->addAction(mActions[LC_PIECE_VIEW_SELECTED_MODEL]);
	PieceMenu->addAction(mActions[LC_PIECE_INLINE_SELECTED_MODELS]);
	PieceMenu->addAction(mActions[LC_PIECE_MOVE_SELECTION_TO_MODEL]);
	PieceMenu->addSeparator();
	PieceMenu->addAction(mActions[LC_PIECE_GROUP]);
	PieceMenu->addAction(mActions[LC_PIECE_UNGROUP]);
	PieceMenu->addAction(mActions[LC_PIECE_GROUP_REMOVE]);
	PieceMenu->addAction(mActions[LC_PIECE_GROUP_ADD]);
	PieceMenu->addAction(mActions[LC_PIECE_GROUP_EDIT]);
	PieceMenu->addSeparator();
	PieceMenu->addAction(mActions[LC_PIECE_HIDE_SELECTED]);
	PieceMenu->addAction(mActions[LC_PIECE_HIDE_UNSELECTED]);
	PieceMenu->addAction(mActions[LC_PIECE_UNHIDE_SELECTED]);
	PieceMenu->addAction(mActions[LC_PIECE_UNHIDE_ALL]);

	QMenu* ModelMenu = menuBar()->addMenu(tr("Sub&model"));
	ModelMenu->addAction(mActions[LC_MODEL_PROPERTIES]);
	ModelMenu->addAction(mActions[LC_MODEL_NEW]);
	ModelMenu->addAction(mActions[LC_MODEL_LIST]);
	ModelMenu->addSeparator();
	for (int ModelIdx = LC_MODEL_FIRST; ModelIdx <= LC_MODEL_LAST; ModelIdx++)
		ModelMenu->addAction(mActions[ModelIdx]);

	QMenu* HelpMenu = menuBar()->addMenu(tr("&Help"));
	HelpMenu->addAction(mActions[LC_HELP_HOMEPAGE]);
	HelpMenu->addAction(mActions[LC_HELP_BUG_REPORT]);
#if !LC_DISABLE_UPDATE_CHECK
	HelpMenu->addAction(mActions[LC_HELP_UPDATES]);
#endif
#ifndef Q_OS_MACOS
	HelpMenu->addSeparator();
#endif
	HelpMenu->addAction(mActions[LC_HELP_ABOUT]);
}

void lcMainWindow::CreateToolBars()
{
    mSelectionModeMenu = new QMenu(tr("&Selection Mode"), this);
	for (int ModeIdx = LC_EDIT_SELECTION_MODE_FIRST; ModeIdx <= LC_EDIT_SELECTION_MODE_LAST; ModeIdx++)
		mSelectionModeMenu->addAction(mActions[ModeIdx]);

    QAction* SelectionModeAction = new QAction(tr("&Selection Mode"), this);
	SelectionModeAction->setStatusTip(tr("Change selection mode"));
	SelectionModeAction->setIcon(QIcon(":/resources/action_select.png"));
	SelectionModeAction->setMenu(mSelectionModeMenu);

    mTransformMenu = new QMenu(tr("Transform and Rotat&e"), this);
	mTransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_RELATIVE]);
	mTransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_ABSOLUTE]);
	mTransformMenu->addSeparator();
	mTransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_TOGETHER]);
	mTransformMenu->addAction(mActions[LC_EDIT_TRANSFORM_SEPARATELY]);

    QAction* TransformAction = new QAction(tr("Transform and Rotat&e"), this);
	TransformAction->setStatusTip(tr("Transform Options"));
	TransformAction->setIcon(QIcon(":/resources/edit_transform_relative.png"));
	TransformAction->setMenu(mTransformMenu);

	QMenu* SnapXYMenu = new QMenu(tr("Snap XY"), this);
	for (int actionIdx = LC_EDIT_SNAP_MOVE_XY0; actionIdx <= LC_EDIT_SNAP_MOVE_XY9; actionIdx++)
		SnapXYMenu->addAction(mActions[actionIdx]);

	QMenu* SnapZMenu = new QMenu(tr("Snap Z"), this);
	for (int actionIdx = LC_EDIT_SNAP_MOVE_Z0; actionIdx <= LC_EDIT_SNAP_MOVE_Z9; actionIdx++)
		SnapZMenu->addAction(mActions[actionIdx]);

	QMenu* SnapMenu = new QMenu(tr("Snap Menu"), this);
	SnapMenu->addAction(mActions[LC_EDIT_SNAP_MOVE_TOGGLE]);
	SnapMenu->addSeparator();
	SnapMenu->addMenu(SnapXYMenu);
	SnapMenu->addMenu(SnapZMenu);

	QAction* MoveAction = new QAction(tr("Movement Snap"), this);
	MoveAction->setStatusTip(tr("Snap translations to fixed intervals"));
	MoveAction->setIcon(QIcon(":/resources/edit_snap_move.png"));
	MoveAction->setMenu(SnapMenu);

	QMenu* SnapAngleMenu = new QMenu(tr("Snap Angle Menu"), this);
	SnapAngleMenu->addAction(mActions[LC_EDIT_SNAP_ANGLE_TOGGLE]);
	SnapAngleMenu->addSeparator();
	for (int actionIdx = LC_EDIT_SNAP_ANGLE0; actionIdx <= LC_EDIT_SNAP_ANGLE9; actionIdx++)
		SnapAngleMenu->addAction(mActions[actionIdx]);

	QAction* AngleAction = new QAction(tr("Rotation Snap"), this);
	AngleAction->setStatusTip(tr("Snap rotations to fixed intervals"));
	AngleAction->setIcon(QIcon(":/resources/edit_snap_angle.png"));
	AngleAction->setMenu(SnapAngleMenu);

	mStandardToolBar = addToolBar(tr("Standard"));
	mStandardToolBar->setObjectName("StandardToolbar");
	mStandardToolBar->addAction(mActions[LC_FILE_NEW]);
	mStandardToolBar->addAction(mActions[LC_FILE_OPEN]);
	mStandardToolBar->addAction(mActions[LC_FILE_SAVE]);
	mStandardToolBar->addSeparator();
	mStandardToolBar->addAction(mActions[LC_EDIT_UNDO]);
	mStandardToolBar->addAction(mActions[LC_EDIT_REDO]);
	mStandardToolBar->addSeparator();
	mStandardToolBar->addAction(SelectionModeAction);
	mStandardToolBar->addAction(TransformAction);
	mStandardToolBar->addAction(MoveAction);
	mStandardToolBar->addAction(AngleAction);
	((QToolButton*)mStandardToolBar->widgetForAction(SelectionModeAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)mStandardToolBar->widgetForAction(TransformAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)mStandardToolBar->widgetForAction(MoveAction))->setPopupMode(QToolButton::InstantPopup);
	((QToolButton*)mStandardToolBar->widgetForAction(AngleAction))->setPopupMode(QToolButton::InstantPopup);

	mTimeToolBar = addToolBar(tr("Time"));
	mTimeToolBar->setObjectName("TimeToolbar");
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_FIRST]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_PREVIOUS]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_NEXT]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_LAST]);
	mTimeToolBar->addAction(mActions[LC_VIEW_TIME_ADD_KEYS]);

	mToolsToolBar = addToolBar(tr("Tools"));
	mToolsToolBar->setObjectName("ToolsToolbar");
	insertToolBarBreak(mToolsToolBar);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_INSERT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_LIGHT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_SPOTLIGHT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_CAMERA]);
	mToolsToolBar->addSeparator();
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_SELECT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_MOVE]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ROTATE]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_DELETE]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_PAINT]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_COLOR_PICKER]);
	mToolsToolBar->addSeparator();
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ZOOM]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_PAN]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ROTATE_VIEW]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ROLL]);
	mToolsToolBar->addAction(mActions[LC_EDIT_ACTION_ZOOM_REGION]);
	mToolsToolBar->hide();

	mPartsToolBar = new QDockWidget(tr("Parts"), this);
	mPartsToolBar->setObjectName("PartsToolbar");
	mPartSelectionWidget = new lcPartSelectionWidget(mPartsToolBar);
	mPartsToolBar->setWidget(mPartSelectionWidget);
	addDockWidget(Qt::RightDockWidgetArea, mPartsToolBar);

	mColorsToolBar = new QDockWidget(tr("Colors"), this);
	mColorsToolBar->setObjectName("ColorsToolbar");
	mColorsToolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	mColorList = new lcColorList();
	connect(mColorList, &lcColorList::ColorChanged, this, &lcMainWindow::ColorChanged);

	QWidget* ColorWidget = new QWidget(mColorsToolBar);

	QVBoxLayout* ColorLayout = new QVBoxLayout(ColorWidget);
	ColorLayout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* ColorButtonLayout = new QHBoxLayout();
	ColorButtonLayout->setContentsMargins(0, 0, 0, 0);
	ColorLayout->addLayout(ColorButtonLayout);

	mColorButton = new QToolButton(ColorWidget);
	mColorButton->setAutoRaise(true);
	mColorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	mColorButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
	ColorButtonLayout->addWidget(mColorButton);

	connect(mColorButton, SIGNAL(clicked()), this, SLOT(ColorButtonClicked()));

	ColorLayout->addWidget(mColorList);

	mColorsToolBar->setWidget(ColorWidget);
	addDockWidget(Qt::RightDockWidgetArea, mColorsToolBar);

	mPropertiesToolBar = new QDockWidget(tr("Properties"), this);
	mPropertiesToolBar->setObjectName("PropertiesToolbar");

	QWidget* PropertiesWidget = new QWidget(mPropertiesToolBar);
	QVBoxLayout* PropertiesLayout = new QVBoxLayout(PropertiesWidget);
	PropertiesLayout->setContentsMargins(0, 0, 0, 0);

	mPropertiesWidget = new lcQPropertiesTree(PropertiesWidget);
	PropertiesLayout->addWidget(mPropertiesWidget);

	QHBoxLayout* TransformLayout = new QHBoxLayout;
	QWidget* TransformWidget = new QWidget();
	TransformWidget->setLayout(TransformLayout);

	QToolButton* TransformButton = new QToolButton(TransformWidget);
	TransformButton->setDefaultAction(mActions[LC_EDIT_TRANSFORM]);
	TransformButton->setPopupMode(QToolButton::InstantPopup);
	TransformLayout->addWidget(TransformButton);

	mTransformXEdit = new lcTransformLineEdit();
	TransformLayout->addWidget(mTransformXEdit);
	mTransformYEdit = new lcTransformLineEdit();
	TransformLayout->addWidget(mTransformYEdit);
	mTransformZEdit = new lcTransformLineEdit();
	TransformLayout->addWidget(mTransformZEdit);

	PropertiesLayout->addWidget(TransformWidget);

	connect(mTransformXEdit, SIGNAL(returnPressed()), mActions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));
	connect(mTransformYEdit, SIGNAL(returnPressed()), mActions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));
	connect(mTransformZEdit, SIGNAL(returnPressed()), mActions[LC_EDIT_TRANSFORM], SIGNAL(triggered()));

	mPropertiesToolBar->setWidget(PropertiesWidget);
	addDockWidget(Qt::RightDockWidgetArea, mPropertiesToolBar);

	mTimelineToolBar = new QDockWidget(tr("Timeline"), this);
	mTimelineToolBar->setObjectName("TimelineToolbar");
	mTimelineToolBar->setAcceptDrops(true);

	mTimelineWidget = new lcTimelineWidget(mTimelineToolBar);

	mTimelineToolBar->setWidget(mTimelineWidget);
	addDockWidget(Qt::RightDockWidgetArea, mTimelineToolBar);

	CreatePreviewWidget();

	tabifyDockWidget(mPartsToolBar, mPropertiesToolBar);
	tabifyDockWidget(mPropertiesToolBar, mTimelineToolBar);
	tabifyDockWidget(mTimelineToolBar, mPreviewToolBar);

	connect(mPropertiesToolBar, SIGNAL(topLevelChanged(bool)), this, SLOT(EnableWindowFlags(bool)));
	connect(mTimelineToolBar,   SIGNAL(topLevelChanged(bool)), this, SLOT(EnableWindowFlags(bool)));
	connect(mPartsToolBar,      SIGNAL(topLevelChanged(bool)), this, SLOT(EnableWindowFlags(bool)));
	connect(mColorsToolBar,     SIGNAL(topLevelChanged(bool)), this, SLOT(EnableWindowFlags(bool)));

	mPartsToolBar->raise();
}

lcView* lcMainWindow::CreateView(lcModel* Model)
{
	lcView* NewView = new lcView(lcViewType::View, Model);

	connect(NewView, SIGNAL(CameraChanged()), this, SLOT(ViewCameraChanged()));
	connect(NewView, SIGNAL(FocusReceived()), this, SLOT(ViewFocusReceived()));

	AddView(NewView);

	return NewView;
}

void lcMainWindow::PreviewPiece(const QString& PartId, int ColorCode, bool ShowPreview)
{
	if (ShowPreview)
		mPreviewToolBar->show();

	mPreviewWidget->SetCurrentPiece(PartId, ColorCode);
}

void lcMainWindow::CreatePreviewWidget()
{
	mPreviewWidget  = new lcPreviewDockWidget();

	mPreviewToolBar = new QDockWidget(tr("Preview"), this);
	mPreviewToolBar->setWindowTitle(tr("Preview"));
	mPreviewToolBar->setObjectName("PreviewToolBar");
	mPreviewToolBar->setWidget(mPreviewWidget);
	addDockWidget(Qt::RightDockWidgetArea, mPreviewToolBar);

	connect(mPreviewToolBar, SIGNAL(topLevelChanged(bool)), this, SLOT(EnableWindowFlags(bool)));
}

void lcMainWindow::TogglePreviewWidget(bool Visible)
{
	if (mPreviewToolBar)
	{
		if (Visible)
			mPreviewToolBar->show();
		else
			mPreviewToolBar->hide();
	}
	else if (Visible) 
	{
		CreatePreviewWidget();
	}
}

void lcMainWindow::EnableWindowFlags(bool Detached)
{
	if (Detached)
	{
		QDockWidget* DockWidget = qobject_cast<QDockWidget*>(sender());

		DockWidget->setWindowFlags(Qt::CustomizeWindowHint | Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

		if (isVisible())
			DockWidget->show();
	}
}

class lcElidedLabel : public QFrame
{
public:
	explicit lcElidedLabel(QWidget* Parent = nullptr)
		: QFrame(Parent)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	}

	void setText(const QString& Text)
	{
		mText = Text;
		update();
	}

protected:
	void paintEvent(QPaintEvent* event) override;

	QString mText;
};

void lcElidedLabel::paintEvent(QPaintEvent* event)
{
	QFrame::paintEvent(event);

	QPainter Painter(this);
	QFontMetrics FontMetrics = Painter.fontMetrics();

	int LineSpacing = FontMetrics.lineSpacing();
	int y = 0;

	QTextLayout TextLayout(mText, Painter.font());
	TextLayout.beginLayout();

	for (;;)
	{
		QTextLine Line = TextLayout.createLine();

		if (!Line.isValid())
			break;

		Line.setLineWidth(width());
		int NextLineY = y + LineSpacing;

		if (height() >= NextLineY + LineSpacing)
		{
			Line.draw(&Painter, QPoint(0, y));
			y = NextLineY;
		}
		else
		{
			QString LastLine = mText.mid(Line.textStart());
			QString ElidedLastLine = FontMetrics.elidedText(LastLine, Qt::ElideRight, width());
			Painter.drawText(QPoint(0, y + FontMetrics.ascent()), ElidedLastLine);
			Line = TextLayout.createLine();
			break;
		}
	}

	TextLayout.endLayout();
}

void lcMainWindow::CreateStatusBar()
{
	QStatusBar* StatusBar = new QStatusBar(this);
	setStatusBar(StatusBar);

	mStatusBarLabel = new lcElidedLabel();
	StatusBar->addWidget(mStatusBarLabel, 1);

	mStatusPositionLabel = new QLabel();
	StatusBar->addPermanentWidget(mStatusPositionLabel);

	mStatusSnapLabel = new QLabel();
	StatusBar->addPermanentWidget(mStatusSnapLabel);

	mStatusTimeLabel = new QLabel();
	StatusBar->addPermanentWidget(mStatusTimeLabel);
}

void lcMainWindow::closeEvent(QCloseEvent* Event)
{
	if (SaveProjectIfModified())
	{
		Event->accept();

		QSettings Settings;
		Settings.beginGroup("MainWindow");
		Settings.setValue("Geometry", saveGeometry());
		Settings.setValue("State", saveState());
		mPartSelectionWidget->SaveState(Settings);
		Settings.endGroup();

		gApplication->SaveTabLayout();
	}
	else
		Event->ignore();
}

void lcMainWindow::dragEnterEvent(QDragEnterEvent* Event)
{
	if (Event->mimeData()->hasUrls())
		Event->acceptProposedAction();
}

void lcMainWindow::dropEvent(QDropEvent* Event)
{
	const QMimeData* MimeData = Event->mimeData();
	const QList<QUrl> Urls = MimeData->urls();
	for (const QUrl& Url : Urls)
		if (OpenProject(Url.toLocalFile()))
			break;
}

QMenu* lcMainWindow::createPopupMenu()
{
	QMenu* Menu = new QMenu(this);

	UpdateDockWidgetActions();

	Menu->addAction(mActions[LC_VIEW_TOOLBAR_PARTS]);
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_COLORS]);
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_PROPERTIES]);
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_TIMELINE]);
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_PREVIEW]);
	Menu->addSeparator();
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_STANDARD]);
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_TOOLS]);
	Menu->addAction(mActions[LC_VIEW_TOOLBAR_TIME]);

	return Menu;
}

void lcMainWindow::UpdateDockWidgetActions()
{
	mActions[LC_VIEW_TOOLBAR_PARTS]->setChecked(mPartsToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_COLORS]->setChecked(mColorsToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_PROPERTIES]->setChecked(mPropertiesToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_TIMELINE]->setChecked(mTimelineToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_STANDARD]->setChecked(mStandardToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_TOOLS]->setChecked(mToolsToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_TIME]->setChecked(mTimeToolBar->isVisible());
	mActions[LC_VIEW_TOOLBAR_PREVIEW]->setChecked(mPreviewToolBar->isVisible());
}

void lcMainWindow::UpdateGamepads()
{
#if LC_ENABLE_GAMEPAD
	QDateTime Now = QDateTime::currentDateTime();
	quint64 Elapsed = mLastGamepadUpdate.msecsTo(Now);
	mLastGamepadUpdate = Now;

	if (!gMainWindow)
		return;

	lcView* ActiveView = GetActiveView();
	if (!ActiveView)
		return;

	const QList<int> Gamepads = QGamepadManager::instance()->connectedGamepads();
	if (Gamepads.isEmpty())
		return;

	QGamepad Gamepad(Gamepads[0]);

	float Scale = (float)Elapsed / 20.0f;
	lcVector3 Distance(Scale * Gamepad.axisLeftX(), 0.0f, -Scale * Gamepad.axisLeftY());

	if (fabsf(Distance.LengthSquared()) > 0.01f)
		ActiveView->MoveCamera(Distance);
#endif
}

void lcMainWindow::ModelTabContextMenuRequested(const QPoint& Point)
{
	QMenu* Menu = new QMenu;

	mModelTabWidgetContextMenuIndex = mModelTabWidget->tabBar()->tabAt(Point);

	if (mModelTabWidget->count() > 1)
		Menu->addAction(tr("Close Other Tabs"), this, SLOT(ModelTabCloseOtherTabs()));
	if (mModelTabWidgetContextMenuIndex == mModelTabWidget->currentIndex())
		Menu->addAction(mActions[LC_VIEW_RESET_VIEWS]);

	Menu->exec(QCursor::pos());
	delete Menu;
}

void lcMainWindow::ModelTabCloseOtherTabs()
{
	if (mModelTabWidgetContextMenuIndex == -1)
		return;

	while (mModelTabWidget->count() - 1 > mModelTabWidgetContextMenuIndex)
		delete mModelTabWidget->widget(mModelTabWidgetContextMenuIndex + 1);

	while (mModelTabWidget->count() > 1)
		delete mModelTabWidget->widget(0);
}

void lcMainWindow::ModelTabClosed(int Index)
{
	if (mModelTabWidget->count() != 1)
		delete mModelTabWidget->widget(Index);
	else
		NewProject();
}

void lcMainWindow::ModelTabChanged(int Index)
{
	Project* Project = lcGetActiveProject();
	lcModelTabWidget* CurrentTab = (lcModelTabWidget*)mModelTabWidget->widget(Index);

	Project->SetActiveModel(Project->GetModels().FindIndex(CurrentTab ? CurrentTab->GetModel() : nullptr));
}

void lcMainWindow::ClipboardChanged()
{
	const QString MimeType = QLatin1String("application/vnd.leocad-clipboard");
	const QMimeData* MimeData = QApplication::clipboard()->mimeData();
	QByteArray ClipboardData;

	if (MimeData && MimeData->hasFormat(MimeType))
		ClipboardData = MimeData->data(MimeType);

	gApplication->SetClipboard(ClipboardData);
}

void lcMainWindow::ActionTriggered()
{
	QObject* Action = sender();

	for (int CommandIdx = 0; CommandIdx < LC_NUM_COMMANDS; CommandIdx++)
	{
		if (Action == mActions[CommandIdx])
		{
			HandleCommand((lcCommandId)CommandIdx);
			break;
		}
	}
}

void lcMainWindow::ColorChanged(int ColorIndex)
{
	SetColorIndex(ColorIndex);
}

void lcMainWindow::ColorButtonClicked()
{
	lcModel* ActiveModel = GetActiveModel();

	if (ActiveModel)
		ActiveModel->PaintSelectedPieces();
}

void lcMainWindow::ProjectFileChanged(const QString& Path)
{
	static bool Ignore;

	if (Ignore)
		return;

	QString Text = tr("The file '%1' has been modified by another application, do you want to reload it?").arg(QDir::toNativeSeparators(Path));

	Project* CurrentProject = lcGetActiveProject();

	Ignore = true;

	if (QMessageBox::question(this, tr("File Changed"), Text, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
	{
		Ignore = false;
		CurrentProject->MarkAsModified();
		UpdateTitle();
		return;
	}

	Ignore = false;

	QFileInfo FileInfo(Path);

	if (FileInfo == QFileInfo(CurrentProject->GetFileName()))
	{
		Project* NewProject = new Project;

		if (NewProject->Load(Path, true))
		{
			QByteArray TabLayout = GetTabLayout();
			gApplication->SetProject(NewProject);
			RestoreTabLayout(TabLayout);
			lcView::UpdateAllViews();
		}
	}
	else
	{
		PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(FileInfo.fileName().toLatin1(), CurrentProject, false, true);

		if (Info && Info->IsProject())
			Info->GetProject()->Load(Path, true);
	}
}

void lcMainWindow::Print(QPrinter* Printer)
{
#ifndef QT_NO_PRINTER
	int DocCopies;
	int PageCopies;

	lcInstructions* Instructions = lcGetActiveProject()->GetInstructions();
	const int PageCount = static_cast<int>(Instructions->mPages.size());

	if (Printer->collateCopies())
	{
		DocCopies = 1;
		PageCopies = Printer->supportsMultipleCopies() ? 1 : Printer->copyCount();
	}
	else
	{
		DocCopies = Printer->supportsMultipleCopies() ? 1 : Printer->copyCount();
		PageCopies = 1;
	}

	int FromPage = Printer->fromPage();
	int ToPage = Printer->toPage();
	bool Ascending = true;

	if (FromPage == 0 && ToPage == 0)
	{
		FromPage = 1;
		ToPage = PageCount;
	}

	FromPage = qMax(1, FromPage);
	ToPage = qMin(PageCount, ToPage);

	if (ToPage < FromPage)
		return;

	if (Printer->pageOrder() == QPrinter::LastPageFirst)
	{
		int Tmp = FromPage;
		FromPage = ToPage;
		ToPage = Tmp;
		Ascending = false;
	}

	QRect PageRect = Printer->pageLayout().paintRectPixels(Printer->resolution());
	const int Resolution = Printer->resolution();
	const int Margin = Resolution / 2; // todo: user setting
	QRect MarginRect = QRect(PageRect.left() + Margin, PageRect.top() + Margin, PageRect.width() - Margin * 2, PageRect.height() - Margin * 2);

	QPainter Painter(Printer);
	bool FirstPage = true;
	// TODO: option to print background

	for (int DocCopy = 0; DocCopy < DocCopies; DocCopy++)
	{
		int Page = FromPage;

		for (;;)
		{
			for (int PageCopy = 0; PageCopy < PageCopies; PageCopy++)
			{
				if (Printer->printerState() == QPrinter::Aborted || Printer->printerState() == QPrinter::Error)
					return;

				if (!FirstPage)
					Printer->newPage();
				else
					FirstPage = false;

				int StepWidth = MarginRect.width();
				int StepHeight = MarginRect.height();

				const lcInstructionsPage& PageLayout = Instructions->mPages[Page - 1];
				lcModel* Model = PageLayout.Steps[0].Model;
				lcStep Step = PageLayout.Steps[0].Step;
				QImage Image = Model->GetStepImage(false, StepWidth, StepHeight, Step);

				Painter.drawImage(MarginRect.left(), MarginRect.top(), Image);

				// TODO: add print options somewhere but Qt doesn't allow changes to the page setup dialog
//				DWORD dwPrint = theApp.GetProfileInt("Settings","Print", PRINT_NUMBERS|PRINT_BORDER);

//				if (print text)
				{
					QFont Font("Helvetica", 96);
					Painter.setFont(Font);

					QFontMetrics FontMetrics(Font);

					int TextMargin = Resolution / 2;
					QRect TextRect = QRect(MarginRect.left() + TextMargin, MarginRect.top() + TextMargin, MarginRect.width() - TextMargin * 2, MarginRect.height() - TextMargin * 2);

					Painter.drawText(TextRect, Qt::AlignTop | Qt::AlignLeft, QString::number(Step));
				}
/*
//				if (print border)
				{
					QPen BlackPen(Qt::black, 2);
					Painter.setPen(BlackPen);

					Painter.drawLine(MarginRect.left(), MarginRect.top(), MarginRect.right(), MarginRect.top());
					Painter.drawLine(MarginRect.left(), MarginRect.bottom(), MarginRect.right(), MarginRect.bottom());
					Painter.drawLine(MarginRect.left(), MarginRect.top(), MarginRect.left(), MarginRect.bottom());
					Painter.drawLine(MarginRect.right(), MarginRect.top(), MarginRect.right(), MarginRect.bottom());
				}
*/
				// TODO: print header and footer
			}

			if (Page == ToPage)
				break;

			if (Ascending)
				Page++;
			else
				Page--;
		}
	}
#endif
}

void lcMainWindow::ShowUpdatesDialog()
{
	lcQUpdateDialog Dialog(this, false);
	Dialog.exec();
}

void lcMainWindow::ShowAboutDialog()
{
	lcAboutDialog Dialog(this);
	Dialog.exec();
}

void lcMainWindow::ShowHTMLDialog()
{
	lcHTMLExportOptions Options(lcGetActiveProject());

	lcQHTMLDialog Dialog(this, &Options);
	if (Dialog.exec() != QDialog::Accepted)
		return;

	Options.SaveDefaults();
	lcGetActiveProject()->ExportHTML(Options);
}

void lcMainWindow::ShowRenderDialog(int Command)
{
	lcRenderDialog Dialog(this, Command);
	Dialog.exec();
}

void lcMainWindow::ShowInstructionsDialog()
{
	lcInstructionsDialog* Dialog = new lcInstructionsDialog(this, lcGetActiveProject());
	Dialog->setWindowModality(Qt::ApplicationModal);
	Dialog->setAttribute(Qt::WA_DeleteOnClose);
	Dialog->show();
}

void lcMainWindow::ShowPrintDialog()
{
#ifndef QT_NO_PRINTER
	int PageCount = static_cast<int>(lcGetActiveProject()->GetInstructions()->mPages.size());

	QPrinter Printer(QPrinter::HighResolution);
	Printer.setFromTo(1, PageCount + 1);

	QPrintDialog PrintDialog(&Printer, this);

	if (PrintDialog.exec() == QDialog::Accepted)
		Print(&Printer);
#endif
}

void lcMainWindow::SetShadingMode(lcShadingMode ShadingMode)
{
	lcGetPreferences().mShadingMode = ShadingMode;

	UpdateShadingMode();

	lcView::UpdateAllViews();

	if (mPartSelectionWidget)
		mPartSelectionWidget->Redraw();
}

void lcMainWindow::SetSelectionMode(lcSelectionMode SelectionMode)
{
	mSelectionMode = SelectionMode;

	UpdateSelectionMode();
}

void lcMainWindow::ToggleViewSphere()
{
	lcGetPreferences().mViewSphereEnabled = !lcGetPreferences().mViewSphereEnabled;

	lcView::UpdateAllViews();
}

void lcMainWindow::ToggleAxisIcon()
{
	lcGetPreferences().mDrawAxes = !lcGetPreferences().mDrawAxes;

	lcView::UpdateAllViews();
}

void lcMainWindow::ToggleGrid()
{
	lcGetPreferences().mGridEnabled = !lcGetPreferences().mGridEnabled;

	lcView::UpdateAllViews();
}

void lcMainWindow::ToggleFadePreviousSteps()
{
	lcGetPreferences().mFadeSteps = !lcGetPreferences().mFadeSteps;

	lcView::UpdateAllViews();
}

QByteArray lcMainWindow::GetTabLayout()
{
	QByteArray TabLayoutData;
	QDataStream DataStream(&TabLayoutData, QIODevice::WriteOnly);

	DataStream << (quint32)LC_TAB_LAYOUT_VERSION;
	qint32 NumTabs = mModelTabWidget->count();
	DataStream << NumTabs;
	DataStream << ((lcModelTabWidget*)mModelTabWidget->currentWidget())->GetModel()->GetProperties().mFileName;

	for (int TabIdx = 0; TabIdx < NumTabs; TabIdx++)
	{
		lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

		DataStream << TabWidget->GetModel()->GetProperties().mFileName;

		std::function<void (QWidget*)> SaveWidget = [&DataStream, &SaveWidget, &TabWidget](QWidget* Widget)
		{
			if (Widget->metaObject() == &lcViewWidget::staticMetaObject)
			{
				lcView* CurrentView = ((lcViewWidget*)Widget)->GetView();

				DataStream << (qint32)0;
				DataStream << (qint32)(TabWidget->GetActiveView() == CurrentView ? 1 : 0);

				const lcCamera* Camera = CurrentView->GetCamera();

				if (Camera->IsSimple())
				{
					DataStream << (qint32)0;
					DataStream << Camera->m_fovy;
					DataStream << Camera->m_zNear;
					DataStream << Camera->m_zFar;
					DataStream << Camera->mPosition;
					DataStream << Camera->mTargetPosition;
					DataStream << Camera->mUpVector;
				}
				else
				{
					DataStream << (qint32)1;
					DataStream << Camera->GetName();
				}
			}
			else
			{
				QSplitter* Splitter = (QSplitter*)Widget;

				DataStream << (qint32)(Splitter->orientation() == Qt::Horizontal ? 1 : 2);
				DataStream << Splitter->sizes();

				SaveWidget(Splitter->widget(0));
				SaveWidget(Splitter->widget(1));
			}
		};

		QLayout* TabLayout = TabWidget->layout();
		SaveWidget(TabLayout->itemAt(0)->widget());
	}

	return TabLayoutData;
}

void lcMainWindow::RestoreTabLayout(const QByteArray& TabLayout)
{
	if (TabLayout.isEmpty())
		return;

	QDataStream DataStream(TabLayout);

	quint32 Version;
	DataStream >> Version;

	if (Version != LC_TAB_LAYOUT_VERSION)
		return;

	qint32 NumTabs;
	DataStream >> NumTabs;
	QString CurrentTabName;
	DataStream >> CurrentTabName;

	RemoveAllModelTabs();

	for (int TabIdx = 0; TabIdx < NumTabs; TabIdx++)
	{
		QString ModelName;
		DataStream >> ModelName;

		lcModel* Model = lcGetActiveProject()->GetModel(ModelName);
		lcModelTabWidget* TabWidget = nullptr;

		if (Model)
		{
			SetCurrentModelTab(Model);
			TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(mModelTabWidget->count() - 1);
		}

		QWidget* ActiveWidget = nullptr;

		std::function<void(QWidget*)> LoadWidget = [&DataStream, &LoadWidget, &ActiveWidget, this](QWidget* ParentWidget)
		{
			qint32 WidgetType;
			DataStream >> WidgetType;

			if (WidgetType == 0)
			{
				qint32 IsActive;
				DataStream >> IsActive;

				if (IsActive)
					ActiveWidget = ParentWidget;

				qint32 CameraType;
				DataStream >> CameraType;

				lcView* CurrentView = nullptr;

				if (ParentWidget)
					CurrentView = ((lcViewWidget*)ParentWidget)->GetView();

				if (CameraType == 0)
				{
					float FoV, ZNear, ZFar;
					lcVector3 Position, TargetPosition, UpVector;

					DataStream >> FoV;
					DataStream >> ZNear;
					DataStream >> ZFar;
					DataStream >> Position;
					DataStream >> TargetPosition;
					DataStream >> UpVector;

					if (CurrentView)
					{
						lcCamera* Camera = CurrentView->GetCamera();
						if (!std::isnan(FoV))
							Camera->m_fovy = FoV;
						if (!std::isnan(ZNear))
							Camera->m_zNear = ZNear;
						if (!std::isnan(ZFar))
							Camera->m_zFar = ZFar;
						if (!Position.IsNan() && !TargetPosition.IsNan() && !UpVector.IsNan())
						{
							Camera->mPosition = Position;
							Camera->mTargetPosition = TargetPosition;
							Camera->mUpVector = UpVector;
						}
						Camera->UpdatePosition(1);
					}
				}
				else
				{
					QByteArray CameraName;
					DataStream >> CameraName;

					if (CurrentView)
						CurrentView->SetCamera(CameraName);
				}
			}
			else
			{
				QList<int> Sizes;
				DataStream >> Sizes;

				if (ParentWidget)
				{
					ParentWidget->setFocus();

					if (WidgetType == 1)
						SplitVertical();
					else
						SplitHorizontal();

					QSplitter* Splitter = (QSplitter*)ParentWidget->parentWidget();
					Splitter->setSizes(Sizes);

					LoadWidget(Splitter->widget(0));
					LoadWidget(Splitter->widget(1));
				}
				else
				{
					LoadWidget(nullptr);
					LoadWidget(nullptr);
				}
			}
		};

		LoadWidget(TabWidget ? TabWidget->layout()->itemAt(0)->widget() : nullptr);

		if (ActiveWidget && TabWidget)
			ActiveWidget->setFocus();
	}

	if (!mModelTabWidget->count())
		lcGetActiveProject()->SetActiveModel(0);
	else
		lcGetActiveProject()->SetActiveModel(CurrentTabName);
}

void lcMainWindow::RemoveAllModelTabs()
{
	while (mModelTabWidget->count())
		delete mModelTabWidget->widget(0);
}

void lcMainWindow::CloseCurrentModelTab()
{
	if (mModelTabWidget->count() > 1)
		delete mModelTabWidget->currentWidget();
	else
		NewProject();
}

void lcMainWindow::SetCurrentModelTab(lcModel* Model)
{
	for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); TabIdx++)
	{
		lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);

		if (TabWidget->GetModel() == Model)
		{
			mModelTabWidget->setCurrentIndex(TabIdx);
			return;
		}
	}

	lcModelTabWidget* TabWidget = new lcModelTabWidget(Model);
	mModelTabWidget->addTab(TabWidget, Model->GetProperties().mFileName);

	QVBoxLayout* CentralLayout = new QVBoxLayout(TabWidget);
	CentralLayout->setContentsMargins(0, 0, 0, 0);

	lcView* NewView = CreateView(Model);
	lcViewWidget* ViewWidget = new lcViewWidget(TabWidget, NewView);
	CentralLayout->addWidget(ViewWidget);

	mModelTabWidget->setCurrentWidget(TabWidget);

	ViewWidget->show();
	ViewWidget->setFocus();
	NewView->ZoomExtents();
}

void lcMainWindow::AddView(lcView* View)
{
	lcModelTabWidget* TabWidget = GetTabWidgetForModel(View->GetModel());

	if (!TabWidget)
		return;

	if (!TabWidget->GetActiveView())
	{
		TabWidget->SetActiveView(View);
		UpdatePerspective();
	}
}

void lcMainWindow::RemoveView(lcView* View)
{
	lcModelTabWidget* TabWidget = GetTabForView(View);

	if (TabWidget)
		TabWidget->RemoveView(View);
}

void lcMainWindow::SetActiveView(lcView* ActiveView)
{
	lcModelTabWidget* TabWidget = GetTabForView(ActiveView);

	if (!TabWidget)
		return;

	lcView* CurrentActiveView = TabWidget->GetActiveView();

	if (CurrentActiveView == ActiveView)
		return;

	if (CurrentActiveView)
		CurrentActiveView->SetTopSubmodelActive();

	TabWidget->SetActiveView(ActiveView);

	UpdateCameraMenu();
	UpdatePerspective();
}

void lcMainWindow::SetTool(lcTool Tool)
{
	mTool = Tool;

	QAction* Action = mActions[LC_EDIT_ACTION_FIRST + static_cast<int>(mTool)];

	if (Action)
		Action->setChecked(true);

	lcView::UpdateAllViews();
}

void lcMainWindow::SetColorIndex(int ColorIndex)
{
	mColorIndex = ColorIndex;

	if (mPartSelectionWidget)
		mPartSelectionWidget->SetColorIndex(ColorIndex);

	UpdateColor();
}

void lcMainWindow::SetMoveSnapEnabled(bool Enabled)
{
	mMoveSnapEnabled = Enabled;
	UpdateSnap();
}

void lcMainWindow::SetAngleSnapEnabled(bool Enabled)
{
	mAngleSnapEnabled = Enabled;
	UpdateSnap();
}

void lcMainWindow::SetMoveXYSnapIndex(int Index)
{
	mMoveXYSnapIndex = Index;
	UpdateSnap();
}

void lcMainWindow::SetMoveZSnapIndex(int Index)
{
	mMoveZSnapIndex = Index;
	UpdateSnap();
}

void lcMainWindow::SetAngleSnapIndex(int Index)
{
	mAngleSnapIndex = Index;
	UpdateSnap();
}

void lcMainWindow::SetRelativeTransform(bool RelativeTransform)
{
	mRelativeTransform = RelativeTransform;

	UpdateLockSnap();
	lcView::UpdateAllViews();
}

void lcMainWindow::SetSeparateTransform(bool SelectionTransform)
{
	mLocalTransform = SelectionTransform;

	UpdateLockSnap();
}

void lcMainWindow::SetTransformType(lcTransformType TransformType)
{
	if (TransformType < lcTransformType::First || TransformType >= lcTransformType::Count)
		return;

	mTransformType = TransformType;

	constexpr const char* IconNames[] =
	{
		":/resources/edit_transform_absolute_translation.png",
		":/resources/edit_transform_relative_translation.png",
		":/resources/edit_transform_absolute_rotation.png",
		":/resources/edit_transform_relative_rotation.png"
	};

	LC_ARRAY_SIZE_CHECK(IconNames, lcTransformType::Count);

	int TransformIndex = static_cast<int>(TransformType);
	mActions[LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION + TransformIndex]->setChecked(true);
	mActions[LC_EDIT_TRANSFORM]->setIcon(QIcon(IconNames[TransformIndex]));
}

void lcMainWindow::SetCurrentPieceInfo(PieceInfo* Info)
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	if (mCurrentPieceInfo)
		Library->ReleasePieceInfo(mCurrentPieceInfo);

	mCurrentPieceInfo = Info;

	if (mCurrentPieceInfo)
		Library->LoadPieceInfo(mCurrentPieceInfo, true, true);
}

lcVector3 lcMainWindow::GetTransformAmount()
{
	lcVector3 Transform;

	Transform.x = lcParseValueLocalized(mTransformXEdit->text());
	Transform.y = lcParseValueLocalized(mTransformYEdit->text());
	Transform.z = lcParseValueLocalized(mTransformZEdit->text());

	return Transform;
}

void lcMainWindow::SplitView(Qt::Orientation Orientation)
{
	QWidget* Focus = focusWidget();

	if (Focus->metaObject() != &lcViewWidget::staticMetaObject)
		return;

	QWidget* Parent = Focus->parentWidget();
	QSplitter* Splitter;
	QList<int> Sizes;

	if (Parent->metaObject() == &lcModelTabWidget::staticMetaObject)
	{
		Splitter = new QSplitter(Orientation, Parent);
		Parent->layout()->addWidget(Splitter);
		Splitter->addWidget(Focus);
		Splitter->addWidget(new lcViewWidget(mModelTabWidget->currentWidget(), CreateView(GetCurrentTabModel())));
	}
	else
	{
		QSplitter* ParentSplitter = (QSplitter*)Parent;
		Sizes = ParentSplitter->sizes();
		int FocusIndex = ParentSplitter->indexOf(Focus);

		Splitter = new QSplitter(Orientation, Parent);
		ParentSplitter->insertWidget(FocusIndex, Splitter);
		Splitter->addWidget(Focus);
		Splitter->addWidget(new lcViewWidget(mModelTabWidget->currentWidget(), CreateView(GetCurrentTabModel())));

		ParentSplitter->setSizes(Sizes);
	}

	Sizes.clear();
	Sizes.append(10);
	Sizes.append(10);
	Splitter->setSizes(Sizes);
	Focus->setFocus();
}

void lcMainWindow::SplitHorizontal()
{
	SplitView(Qt::Vertical);
}

void lcMainWindow::SplitVertical()
{
	SplitView(Qt::Horizontal);
}

void lcMainWindow::RemoveActiveView()
{
	QWidget* Focus = focusWidget();

	if (Focus->metaObject() != &lcViewWidget::staticMetaObject)
		return;

	QWidget* Parent = Focus->parentWidget();

	if (Parent->metaObject() == &lcModelTabWidget::staticMetaObject)
		return;

	QWidget* ParentParentWidget = Parent->parentWidget();
	QSplitter* ParentSplitter = (QSplitter*)Parent;
	int FocusIndex = ParentSplitter->indexOf(Focus);
	QWidget* OtherWidget = ParentSplitter->widget(!FocusIndex);

	if (ParentParentWidget->metaObject() == &lcModelTabWidget::staticMetaObject)
	{
		QLayout* CentralLayout = ParentParentWidget->layout();

		CentralLayout->addWidget(OtherWidget);
		CentralLayout->removeWidget(Parent);
	}
	else
	{
		QSplitter* ParentParentSplitter = (QSplitter*)ParentParentWidget;
		QList<int> Sizes = ParentParentSplitter->sizes();

		int ParentIndex = ParentParentSplitter->indexOf(Parent);
		Parent->setParent(nullptr);
		ParentParentSplitter->insertWidget(ParentIndex, OtherWidget);

		ParentParentSplitter->setSizes(Sizes);
	}

	Parent->deleteLater();

	if (OtherWidget->metaObject() != &lcViewWidget::staticMetaObject)
	{
		lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->currentWidget();

		if (TabWidget)
			OtherWidget = TabWidget->GetAnyViewWidget();
	}

	OtherWidget->setFocus();
}

void lcMainWindow::ResetViews()
{
	lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->currentWidget();

	if (!TabWidget)
		return;

	QLayout* TabLayout = TabWidget->layout();
	QWidget* TopWidget = TabLayout->itemAt(0)->widget();
	TopWidget->deleteLater();

	lcView* NewView = CreateView(TabWidget->GetModel());
	lcViewWidget* ViewWidget = new lcViewWidget(TabWidget, NewView);
	TabLayout->addWidget(ViewWidget);

	ViewWidget->show();
	ViewWidget->setFocus();
	NewView->ZoomExtents();
}

void lcMainWindow::ToggleDockWidget(QWidget* DockWidget)
{
	if (DockWidget->isHidden())
		DockWidget->show();
	else
		DockWidget->hide();
}

void lcMainWindow::TogglePrintPreview()
{
#ifndef QT_NO_PRINTER
	// todo: print preview inside main window

	int PageCount = static_cast<int>(lcGetActiveProject()->GetInstructions()->mPages.size());

	QPrinter Printer(QPrinter::ScreenResolution);
	Printer.setFromTo(1, PageCount + 1);

	QPrintPreviewDialog Preview(&Printer, this);
	connect(&Preview, SIGNAL(paintRequested(QPrinter*)), SLOT(Print(QPrinter*)));
	Preview.exec();
#endif
}

void lcMainWindow::ToggleFullScreen()
{
	// todo: hide toolbars and menu
	// todo: create fullscreen toolbar or support esc key to go back
	if (isFullScreen())
		showNormal();
	else
		showFullScreen();
}

void lcMainWindow::AddRecentFile(const QString& FileName)
{
	QString SavedName = FileName;
	int FileIdx;

	QFileInfo FileInfo(FileName);

	for (FileIdx = 0; FileIdx < LC_MAX_RECENT_FILES; FileIdx++)
		if (QFileInfo(mRecentFiles[FileIdx]) == FileInfo)
			break;

	for (FileIdx = lcMin(FileIdx, LC_MAX_RECENT_FILES - 1); FileIdx > 0; FileIdx--)
		mRecentFiles[FileIdx] = mRecentFiles[FileIdx - 1];

	mRecentFiles[0] = SavedName;

	UpdateRecentFiles();
}

void lcMainWindow::RemoveRecentFile(int FileIndex)
{
	for (int FileIdx = FileIndex; FileIdx < LC_MAX_RECENT_FILES - 1; FileIdx++)
		mRecentFiles[FileIdx] = mRecentFiles[FileIdx + 1];

	mRecentFiles[LC_MAX_RECENT_FILES - 1].clear();

	UpdateRecentFiles();
}

void lcMainWindow::UpdateSelectedObjects(bool SelectionChanged)
{
	int Flags = 0;
	lcArray<lcObject*> Selection;
	lcObject* Focus = nullptr;

	lcModel* ActiveModel = GetActiveModel();
	if (ActiveModel)
		ActiveModel->GetSelectionInformation(&Flags, Selection, &Focus);

	if (SelectionChanged)
	{
		mTimelineWidget->UpdateSelection();

		mActions[LC_EDIT_CUT]->setEnabled(Flags & LC_SEL_SELECTED);
		mActions[LC_EDIT_COPY]->setEnabled(Flags & LC_SEL_SELECTED);
		mActions[LC_EDIT_FIND]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_FIND_NEXT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_FIND_PREVIOUS]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_FIND_ALL]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_REPLACE]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_REPLACE_NEXT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_REPLACE_ALL]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_SELECT_INVERT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_SELECT_BY_NAME]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_EDIT_SELECT_NONE]->setEnabled(Flags & LC_SEL_SELECTED);
		mActions[LC_EDIT_SELECT_ALL]->setEnabled(Flags & LC_SEL_UNSELECTED);

		mActions[LC_PIECE_DELETE]->setEnabled(Flags & LC_SEL_SELECTED);
		mActions[LC_PIECE_DUPLICATE]->setEnabled(Flags & LC_SEL_SELECTED);
        mActions[LC_PIECE_REPLACE]->setEnabled(Flags & LC_SEL_SELECTED);
        mActions[LC_PIECE_PAINT_SELECTED]->setEnabled(Flags & LC_SEL_PIECE);
		mActions[LC_PIECE_RESET_PIVOT_POINT]->setEnabled(Flags & LC_SEL_SELECTED);
		mActions[LC_PIECE_REMOVE_KEY_FRAMES]->setEnabled(Flags & LC_SEL_SELECTED);
		mActions[LC_PIECE_ARRAY]->setEnabled(Flags & LC_SEL_PIECE);
		mActions[LC_PIECE_CONTROL_POINT_INSERT]->setEnabled(Flags & LC_SEL_CAN_ADD_CONTROL_POINT);
		mActions[LC_PIECE_CONTROL_POINT_REMOVE]->setEnabled(Flags & LC_SEL_CAN_REMOVE_CONTROL_POINT);
		mActions[LC_PIECE_HIDE_SELECTED]->setEnabled(Flags & LC_SEL_VISIBLE_SELECTED);
		mActions[LC_PIECE_HIDE_UNSELECTED]->setEnabled(Flags & LC_SEL_UNSELECTED);
		mActions[LC_PIECE_UNHIDE_SELECTED]->setEnabled(Flags & LC_SEL_HIDDEN_SELECTED);
		mActions[LC_PIECE_UNHIDE_ALL]->setEnabled(Flags & LC_SEL_HIDDEN);
		mActions[LC_PIECE_VIEW_SELECTED_MODEL]->setEnabled(Flags & LC_SEL_MODEL_SELECTED);
		mActions[LC_PIECE_MOVE_SELECTION_TO_MODEL]->setEnabled(Flags & LC_SEL_PIECE);
		mActions[LC_PIECE_INLINE_SELECTED_MODELS]->setEnabled(Flags & LC_SEL_MODEL_SELECTED);
		mActions[LC_PIECE_EDIT_SELECTED_SUBMODEL]->setEnabled(Flags & LC_SEL_MODEL_SELECTED);
		mActions[LC_PIECE_GROUP]->setEnabled(Flags & LC_SEL_CAN_GROUP);
		mActions[LC_PIECE_UNGROUP]->setEnabled(Flags & LC_SEL_GROUPED);
		mActions[LC_PIECE_GROUP_ADD]->setEnabled((Flags & (LC_SEL_GROUPED | LC_SEL_FOCUS_GROUPED)) == LC_SEL_GROUPED);
		mActions[LC_PIECE_GROUP_REMOVE]->setEnabled(Flags & LC_SEL_FOCUS_GROUPED);
		mActions[LC_PIECE_GROUP_EDIT]->setEnabled((Flags & LC_SEL_NO_PIECES) == 0);
		mActions[LC_PIECE_SHOW_EARLIER]->setEnabled(Flags & LC_SEL_PIECE); // FIXME: disable if current step is 1
		mActions[LC_PIECE_SHOW_LATER]->setEnabled(Flags & LC_SEL_PIECE);
		mActions[LC_TIMELINE_MOVE_SELECTION]->setEnabled(Flags & LC_SEL_PIECE);
		mActions[LC_TIMELINE_MOVE_SELECTION_BEFORE]->setEnabled(Flags & LC_SEL_PIECE);
		mActions[LC_TIMELINE_MOVE_SELECTION_AFTER]->setEnabled(Flags & LC_SEL_PIECE);

		mActions[LC_PIECE_EDIT_END_SUBMODEL]->setEnabled(GetCurrentTabModel() != ActiveModel);
	}

	mPropertiesWidget->Update(Selection, Focus);

	QString Message;

	if ((Selection.GetSize() == 1) && Focus)
	{
		if (Focus->IsPiece())
			Message = tr("%1 (ID: %2)").arg(Focus->GetName(), ((lcPiece*)Focus)->GetID());
		else
			Message = Focus->GetName();
	}
	else if (Selection.GetSize() > 0)
	{
		Message = tr("%n Object(s) selected", "", Selection.GetSize());

		if (Focus && Focus->IsPiece())
		{
			Message.append(tr(" - %1 (ID: %2)").arg(Focus->GetName(), ((lcPiece*)Focus)->GetID()));

			const lcGroup* Group = ((lcPiece*)Focus)->GetGroup();
			if (Group && !Group->mName.isEmpty())
				Message.append(tr(" in group '%1'").arg(Group->mName));
		}
	}

	mStatusBarLabel->setText(Message);
	lcVector3 Position;
	lcGetActiveModel()->GetFocusPosition(Position);

	QString Label("X: %1 Y: %2 Z: %3");
	Label = Label.arg(QLocale::system().toString(Position[0], 'f', 2), QLocale::system().toString(Position[1], 'f', 2), QLocale::system().toString(Position[2], 'f', 2));
	mStatusPositionLabel->setText(Label);
}

void lcMainWindow::UpdateTimeline(bool Clear, bool UpdateItems)
{
	mTimelineWidget->Update(Clear, UpdateItems);
}

void lcMainWindow::UpdatePaste(bool Enabled)
{
	if (mActions[LC_EDIT_PASTE])
		mActions[LC_EDIT_PASTE]->setEnabled(Enabled);

	if (mActions[LC_EDIT_PASTE_STEPS])
		mActions[LC_EDIT_PASTE_STEPS]->setEnabled(Enabled);
}

void lcMainWindow::UpdateCurrentStep()
{
	lcModel* Model = lcGetActiveModel();
	lcStep CurrentStep = Model->GetCurrentStep();
	lcStep LastStep = Model->GetLastStep();

	mActions[LC_VIEW_TIME_FIRST]->setEnabled(CurrentStep != 1);
	mActions[LC_VIEW_TIME_PREVIOUS]->setEnabled(CurrentStep > 1);
	mActions[LC_VIEW_TIME_NEXT]->setEnabled(CurrentStep < LC_STEP_MAX);
	mActions[LC_VIEW_TIME_LAST]->setEnabled(CurrentStep != LastStep);

	mStatusTimeLabel->setText(QString(tr("Step %1")).arg(QString::number(CurrentStep)));
}

void lcMainWindow::SetAddKeys(bool AddKeys)
{
	QAction* Action = mActions[LC_VIEW_TIME_ADD_KEYS];

	if (Action)
		Action->setChecked(AddKeys);

	mAddKeys = AddKeys;
}

void lcMainWindow::UpdateLockSnap()
{
	if (GetRelativeTransform())
		mActions[LC_EDIT_TRANSFORM_RELATIVE]->setChecked(true);
	else
		mActions[LC_EDIT_TRANSFORM_ABSOLUTE]->setChecked(true);

	if (GetSeparateTransform())
		mActions[LC_EDIT_TRANSFORM_SEPARATELY]->setChecked(true);
	else
		mActions[LC_EDIT_TRANSFORM_TOGETHER]->setChecked(true);

	UpdateSnap();
}

void lcMainWindow::UpdateSnap()
{
	mActions[LC_EDIT_SNAP_MOVE_TOGGLE]->setChecked(mMoveSnapEnabled);
	mActions[LC_EDIT_SNAP_ANGLE_TOGGLE]->setChecked(mAngleSnapEnabled);
	mActions[LC_EDIT_SNAP_MOVE_XY0 + mMoveXYSnapIndex]->setChecked(true);
	mActions[LC_EDIT_SNAP_MOVE_Z0 + mMoveZSnapIndex]->setChecked(true);
	mActions[LC_EDIT_SNAP_ANGLE0 + mAngleSnapIndex]->setChecked(true);

	QString Relative = mRelativeTransform ? tr("Rel") : tr("Abs");
	mStatusSnapLabel->setText(QString(tr(" M: %1 %2 R: %3 %4 ")).arg(GetMoveXYSnapText(), GetMoveZSnapText(), GetAngleSnapText(), Relative));
}

void lcMainWindow::UpdateColor()
{
	QPixmap Pixmap(14, 14);
	Pixmap.fill(QColor::fromRgbF(gColorList[mColorIndex].Value[0], gColorList[mColorIndex].Value[1], gColorList[mColorIndex].Value[2]));

	mColorButton->setIcon(Pixmap);
	mColorButton->setText(QString("  ") + gColorList[mColorIndex].Name);
	mColorList->SetCurrentColor(mColorIndex);
}

void lcMainWindow::UpdateUndoRedo(const QString& UndoText, const QString& RedoText)
{
	QAction* UndoAction = mActions[LC_EDIT_UNDO];
	QAction* RedoAction = mActions[LC_EDIT_REDO];

	if (!UndoText.isEmpty())
	{
		UndoAction->setEnabled(true);
		UndoAction->setText(QString(tr("&Undo %1")).arg(UndoText));
	}
	else
	{
		UndoAction->setEnabled(false);
		UndoAction->setText(tr("&Undo"));
	}

	if (!RedoText.isEmpty())
	{
		RedoAction->setEnabled(true);
		RedoAction->setText(QString(tr("&Redo %1")).arg(RedoText));
	}
	else
	{
		RedoAction->setEnabled(false);
		RedoAction->setText(tr("&Redo"));
	}
}

void lcMainWindow::ViewFocusReceived()
{
	SetActiveView(qobject_cast<lcView*>(sender()));
}

void lcMainWindow::ViewCameraChanged()
{
	lcView* View = qobject_cast<lcView*>(sender());

	if (!View || !View->IsLastFocused())
		return;

	UpdateCameraMenu();
}

void lcMainWindow::UpdateCameraMenu()
{
	const lcArray<lcCamera*>& Cameras = lcGetActiveModel()->GetCameras();
	lcView* ActiveView = GetActiveView();
	const lcCamera* CurrentCamera = ActiveView ? ActiveView->GetCamera() : nullptr;
	bool CurrentSet = false;

	for (int ActionIdx = LC_VIEW_CAMERA_FIRST; ActionIdx <= LC_VIEW_CAMERA_LAST; ActionIdx++)
	{
		QAction* Action = mActions[ActionIdx];
		int CameraIdx = ActionIdx - LC_VIEW_CAMERA_FIRST;

		if (CameraIdx < Cameras.GetSize())
		{
			if (CurrentCamera == Cameras[CameraIdx])
			{
				Action->setChecked(true);
				CurrentSet = true;
			}

			Action->setText(Cameras[CameraIdx]->GetName());
			Action->setVisible(true);
		}
		else
			Action->setVisible(false);
	}

	if (!CurrentSet)
		mActions[LC_VIEW_CAMERA_NONE]->setChecked(true);

	UpdatePerspective();
}

void lcMainWindow::UpdatePerspective()
{
	lcView* ActiveView = GetActiveView();

	if (ActiveView)
	{
		if (ActiveView->GetCamera()->IsOrtho())
			mActions[LC_VIEW_PROJECTION_ORTHO]->setChecked(true);
		else
			mActions[LC_VIEW_PROJECTION_PERSPECTIVE]->setChecked(true);
	}
}

void lcMainWindow::UpdateShadingMode()
{
	mActions[LC_VIEW_SHADING_FIRST + static_cast<int>(lcGetPreferences().mShadingMode)]->setChecked(true);
}

void lcMainWindow::UpdateSelectionMode()
{
	switch (mSelectionMode)
	{
	case lcSelectionMode::Single:
		mActions[LC_EDIT_SELECTION_SINGLE]->setChecked(true);
		break;

	case lcSelectionMode::Piece:
		mActions[LC_EDIT_SELECTION_PIECE]->setChecked(true);
		break;

	case lcSelectionMode::Color:
		mActions[LC_EDIT_SELECTION_COLOR]->setChecked(true);
		break;

	case lcSelectionMode::PieceColor:
		mActions[LC_EDIT_SELECTION_PIECE_COLOR]->setChecked(true);
		break;
	}
}

void lcMainWindow::UpdateModels()
{
	const lcArray<lcModel*>& Models = lcGetActiveProject()->GetModels();
	lcModel* CurrentModel = lcGetActiveModel();

	for (int ActionIdx = LC_MODEL_FIRST; ActionIdx <= LC_MODEL_LAST; ActionIdx++)
	{
		QAction* Action = mActions[ActionIdx];
		int ModelIdx = ActionIdx - LC_MODEL_FIRST;

		if (ModelIdx < Models.GetSize())
		{
			Action->setChecked(CurrentModel == Models[ModelIdx]);
			Action->setText(QString::fromLatin1("%1%2 %3").arg(ModelIdx < 9 ? QString("&") : QString(), QString::number(ModelIdx + 1), Models[ModelIdx]->GetProperties().mFileName));
			Action->setVisible(true);
		}
		else
			Action->setVisible(false);
	}

	for (int TabIdx = 0; TabIdx < mModelTabWidget->count(); )
	{
		lcModelTabWidget* TabWidget = (lcModelTabWidget*)mModelTabWidget->widget(TabIdx);
		lcModel* Model = TabWidget->GetModel();

		if (!Model)
			TabIdx++;
		else if (Models.FindIndex(Model) != -1)
		{
			mModelTabWidget->setTabText(TabIdx, Model->GetProperties().mFileName);
			TabIdx++;
		}
		else
			delete TabWidget;
	}

	mPartSelectionWidget->UpdateModels();

	if (mCurrentPieceInfo && mCurrentPieceInfo->IsModel())
		if (Models.FindIndex(mCurrentPieceInfo->GetModel()) == -1)
			SetCurrentPieceInfo(nullptr);
}

void lcMainWindow::UpdateCategories()
{
	mPartSelectionWidget->UpdateCategories();
}

void lcMainWindow::UpdateTitle()
{
	setWindowModified(lcGetActiveProject()->IsModified());
	setWindowFilePath(lcGetActiveProject()->GetTitle());
}

void lcMainWindow::UpdateModified(bool Modified)
{
	setWindowModified(Modified);
}

void lcMainWindow::UpdateRecentFiles()
{
	for (int ActionIdx = LC_FILE_RECENT_FIRST; ActionIdx <= LC_FILE_RECENT_LAST; ActionIdx++)
	{
		int FileIdx = ActionIdx - LC_FILE_RECENT_FIRST;
		QAction* Action = mActions[ActionIdx];

		if (!mRecentFiles[FileIdx].isEmpty())
		{
			Action->setText(QString("&%1 %2").arg(QString::number(FileIdx + 1), QDir::toNativeSeparators(mRecentFiles[FileIdx])));
			Action->setVisible(true);
		}
		else
			Action->setVisible(false);
	}

	mActionFileRecentSeparator->setVisible(!mRecentFiles[0].isEmpty());
}

void lcMainWindow::UpdateShortcuts()
{
	for (int ActionIdx = 0; ActionIdx < LC_NUM_COMMANDS; ActionIdx++)
		mActions[ActionIdx]->setShortcut(QKeySequence(gKeyboardShortcuts.mShortcuts[ActionIdx]));
}

void lcMainWindow::NewProject()
{
	if (!SaveProjectIfModified())
		return;

	Project* NewProject = new Project();
	gApplication->SetProject(NewProject);
	lcGetPiecesLibrary()->UnloadUnusedParts();
}

bool lcMainWindow::OpenProject(const QString& FileName)
{
	if (!SaveProjectIfModified())
		return false;

	QString LoadFileName = FileName;

	if (LoadFileName.isEmpty())
	{
		LoadFileName = lcGetActiveProject()->GetFileName();

		if (LoadFileName.isEmpty())
			LoadFileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

		LoadFileName = QFileDialog::getOpenFileName(this, tr("Open Model"), LoadFileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

		if (LoadFileName.isEmpty())
			return false;

		lcSetProfileString(LC_PROFILE_PROJECTS_PATH, QFileInfo(LoadFileName).absolutePath());
	}

    if (LoadFileName.endsWith(".lxf", Qt::CaseInsensitive))
    {
        ImportLDD(LoadFileName);
        return true;
    }
	return OpenProjectFile(LoadFileName);
}

void lcMainWindow::OpenRecentProject(int RecentFileIndex)
{
	if (!SaveProjectIfModified())
		return;

	if (!OpenProjectFile(mRecentFiles[RecentFileIndex]))
		RemoveRecentFile(RecentFileIndex);
}

bool lcMainWindow::OpenProjectFile(const QString& FileName)
{
	Project* NewProject = new Project();

	if (NewProject->Load(FileName, true))
	{
		gApplication->SetProject(NewProject);
		AddRecentFile(FileName);
		lcView::UpdateProjectViews(NewProject);

		return true;
	}

	delete NewProject;
	return false;
}

void lcMainWindow::MergeProject()
{
	QString LoadFileName = lcGetActiveProject()->GetFileName();

	if (LoadFileName.isEmpty())
		LoadFileName = lcGetProfileString(LC_PROFILE_PROJECTS_PATH);

	LoadFileName = QFileDialog::getOpenFileName(this, tr("Merge Model"), LoadFileName, tr("Supported Files (*.lcd *.ldr *.dat *.mpd);;All Files (*.*)"));

	if (LoadFileName.isEmpty())
		return;

	lcSetProfileString(LC_PROFILE_PROJECTS_PATH, QFileInfo(LoadFileName).absolutePath());

	Project* NewProject = new Project();

	if (NewProject->Load(LoadFileName, true))
	{
		int NumModels = NewProject->GetModels().GetSize();

		lcGetActiveProject()->Merge(NewProject);

		if (NumModels == 1)
			QMessageBox::information(this, tr("LeoCAD"), tr("Merged 1 submodel."));
		else
			QMessageBox::information(this, tr("LeoCAD"), tr("Merged %1 submodels.").arg(NumModels));

		UpdateModels();
	}

	delete NewProject;
}

void lcMainWindow::ImportLDD()
{
	if (!SaveProjectIfModified())
		return;

	QString LoadFileName = QFileDialog::getOpenFileName(this, tr("Import"), QString(), tr("LEGO Diginal Designer Files (*.lxf);;All Files (*.*)"));
	if (LoadFileName.isEmpty())
		return;

	Project* NewProject = new Project();

	if (NewProject->ImportLDD(LoadFileName))
	{
		gApplication->SetProject(NewProject);
		lcView::UpdateProjectViews(NewProject);
	}
	else
		delete NewProject;
}

void lcMainWindow::ImportLDD(const QString& FileName)
{
    if (!SaveProjectIfModified())
        return;

    Project* NewProject = new Project();

    if (NewProject->ImportLDD(FileName))
    {
        gApplication->SetProject(NewProject);
        lcView::UpdateProjectViews(NewProject);
    }
    else
        delete NewProject;
}

void lcMainWindow::ImportInventory()
{
	if (!SaveProjectIfModified())
		return;

	lcSetsDatabaseDialog Dialog(this);
	if (Dialog.exec() != QDialog::Accepted)
		return;

	Project* NewProject = new Project();

	if (NewProject->ImportInventory(Dialog.GetSetInventory(), Dialog.GetSetName(), Dialog.GetSetDescription()))
	{
		gApplication->SetProject(NewProject);
		lcView::UpdateProjectViews(NewProject);
	}
	else
		delete NewProject;
}
bool lcMainWindow::SaveProject(const QString& FileName)
{
	QString SaveFileName = FileName;
	Project* Project = lcGetActiveProject();

	if (SaveFileName.isEmpty())
	{
		SaveFileName = Project->GetFileName();

		if (SaveFileName.isEmpty())
			SaveFileName = QFileInfo(QDir(lcGetProfileString(LC_PROFILE_PROJECTS_PATH)), Project->GetTitle()).absoluteFilePath();

		QString Filter = (Project->GetModels().GetSize() > 1) ? tr("Supported Files (*.mpd);;All Files (*.*)") : tr("Supported Files (*.ldr *.dat *.mpd);;All Files (*.*)");

		SaveFileName = QFileDialog::getSaveFileName(this, tr("Save Model"), SaveFileName, Filter);

		if (SaveFileName.isEmpty())
			return false;

		lcSetProfileString(LC_PROFILE_PROJECTS_PATH, QFileInfo(SaveFileName).absolutePath());
	}

	if (QFileInfo(SaveFileName).suffix().toLower() == QLatin1String("lcd"))
	{
		QMessageBox::warning(this, tr("Error"), tr("Saving files in LCD format is no longer supported, please use the LDR or MPD formats instead."));
		return false;
	}

	if (!Project->Save(SaveFileName))
		return false;

	AddRecentFile(SaveFileName);
	UpdateTitle();

	return true;
}

bool lcMainWindow::SaveProjectIfModified()
{
	Project* Project = lcGetActiveProject();
	if (!Project->IsModified())
		return true;

	switch (QMessageBox::question(this, tr("Save Model"), tr("Save changes to '%1'?").arg(Project->GetTitle()), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
	{
	default:
	case QMessageBox::Cancel:
		return false;

	case QMessageBox::Yes:
		if (!SaveProject(Project->GetFileName()))
			return false;
		break;

	case QMessageBox::No:
		break;
	}

	return true;
}

bool lcMainWindow::SetModelFromFocus()
{
	lcObject* FocusObject = GetActiveModel()->GetFocusObject();

	if (!FocusObject || !FocusObject->IsPiece())
		return false;

	lcModel* Model = ((lcPiece*)FocusObject)->mPieceInfo->GetModel();

	if (Model)
	{
		Project* Project = lcGetActiveProject();
		Project->SetActiveModel(Project->GetModels().FindIndex(Model));
		return true;
	}

	return false;
}

void lcMainWindow::SetModelFromSelection()
{
	if (SetModelFromFocus())
		return;

	lcModel* Model = GetActiveModel()->GetFirstSelectedSubmodel();

	if (Model)
	{
		Project* Project = lcGetActiveProject();
		Project->SetActiveModel(Project->GetModels().FindIndex(Model));
	}
}

lcModel* lcMainWindow::GetActiveModel() const
{
	lcView* ActiveView = GetActiveView();
	return ActiveView ? ActiveView->GetActiveModel() : nullptr;
}

lcModelTabWidget* lcMainWindow::GetTabForView(lcView* View) const
{
	QWidget* Widget = View->GetWidget();

	while (Widget)
	{
		lcModelTabWidget* TabWidget = qobject_cast<lcModelTabWidget*>(Widget);

		if (TabWidget)
			return TabWidget;
		else
			Widget = Widget->parentWidget();
	}

	return nullptr;
}

void lcMainWindow::HandleCommand(lcCommandId CommandId)
{
	lcView* ActiveView = GetActiveView();
	lcModel* ActiveModel = ActiveView ? ActiveView->GetActiveModel() : nullptr;

	switch (CommandId)
	{
	case LC_FILE_NEW:
		NewProject();
		break;

	case LC_FILE_OPEN:
		OpenProject(QString());
		break;

	case LC_FILE_MERGE:
		MergeProject();
		break;

	case LC_FILE_SAVE:
		SaveProject(lcGetActiveProject()->GetFileName());
		break;

	case LC_FILE_SAVEAS:
		SaveProject(QString());
		break;

	case LC_FILE_SAVE_IMAGE:
		lcGetActiveProject()->SaveImage();
		break;

	case LC_FILE_IMPORT_LDD:
		ImportLDD();
		break;

	case LC_FILE_IMPORT_INVENTORY:
		ImportInventory();
		break;

	case LC_FILE_EXPORT_3DS:
		lcGetActiveProject()->Export3DStudio(QString());
		break;

	case LC_FILE_EXPORT_COLLADA:
		lcGetActiveProject()->ExportCOLLADA(QString());
		break;

	case LC_FILE_EXPORT_HTML:
		ShowHTMLDialog();
		break;

	case LC_FILE_EXPORT_BRICKLINK:
		lcGetActiveProject()->ExportBrickLink();
		break;

	case LC_FILE_EXPORT_CSV:
		lcGetActiveProject()->ExportCSV(QString());
		break;

	case LC_FILE_EXPORT_POVRAY:
		lcGetActiveProject()->ExportPOVRay(QString());
		break;

	case LC_FILE_EXPORT_WAVEFRONT:
		lcGetActiveProject()->ExportWavefront(QString());
		break;

	case LC_FILE_RENDER_POVRAY:
	case LC_FILE_RENDER_BLENDER:
	case LC_FILE_RENDER_OPEN_IN_BLENDER:
		ShowRenderDialog(CommandId - LC_FILE_RENDER_POVRAY);
		break;

	case LC_FILE_INSTRUCTIONS:
		ShowInstructionsDialog();
		break;

	case LC_FILE_PRINT_PREVIEW:
		TogglePrintPreview();
		break;

	case LC_FILE_PRINT:
		ShowPrintDialog();
		break;

	case LC_FILE_RECENT1:
	case LC_FILE_RECENT2:
	case LC_FILE_RECENT3:
	case LC_FILE_RECENT4:
		OpenRecentProject(CommandId - LC_FILE_RECENT1);
		break;

	case LC_FILE_EXIT:
		close();
		break;

	case LC_EDIT_UNDO:
		if (ActiveModel)
			ActiveModel->UndoAction();
		break;

	case LC_EDIT_REDO:
		if (ActiveModel)
			ActiveModel->RedoAction();
		break;

	case LC_EDIT_CUT:
		if (ActiveModel)
			ActiveModel->Cut();
		break;

	case LC_EDIT_COPY:
		if (ActiveModel)
			ActiveModel->Copy();
		break;

	case LC_EDIT_PASTE:
		if (ActiveModel)
			ActiveModel->Paste(true);
		break;

	case LC_EDIT_PASTE_STEPS:
		if (ActiveModel)
			ActiveModel->Paste(false);
		break;

	case LC_EDIT_FIND:
		if (ActiveView)
			ActiveView->ShowFindReplaceWidget(false);
		break;

	case LC_EDIT_FIND_NEXT:
		if (ActiveModel)
			ActiveModel->FindReplacePiece(true, false);
		break;

	case LC_EDIT_FIND_PREVIOUS:
		if (ActiveModel)
			ActiveModel->FindReplacePiece(false, false);
		break;

	case LC_EDIT_FIND_ALL:
		if (ActiveModel)
			ActiveModel->FindReplacePiece(true, true);
		break;

	case LC_EDIT_REPLACE:
		if (ActiveView)
			ActiveView->ShowFindReplaceWidget(true);
		break;

	case LC_EDIT_REPLACE_ALL:
		if (ActiveModel)
			ActiveModel->FindReplacePiece(true, true);
		break;

	case LC_EDIT_REPLACE_NEXT:
		if (ActiveModel)
			ActiveModel->FindReplacePiece(true, false);
		break;

	case LC_EDIT_SELECT_ALL:
		if (ActiveModel)
			ActiveModel->SelectAllPieces();
		break;

	case LC_EDIT_SELECT_NONE:
		if (ActiveModel)
			ActiveModel->ClearSelection(true);
		break;

	case LC_EDIT_SELECT_INVERT:
		if (ActiveModel)
			ActiveModel->InvertSelection();
		break;

	case LC_EDIT_SELECT_BY_NAME:
		if (ActiveModel)
			ActiveModel->ShowSelectByNameDialog();
		break;

	case LC_EDIT_SELECTION_SINGLE:
		SetSelectionMode(lcSelectionMode::Single);
		break;

	case LC_EDIT_SELECTION_PIECE:
		SetSelectionMode(lcSelectionMode::Piece);
		break;

	case LC_EDIT_SELECTION_COLOR:
		SetSelectionMode(lcSelectionMode::Color);
		break;

	case LC_EDIT_SELECTION_PIECE_COLOR:
		SetSelectionMode(lcSelectionMode::PieceColor);
		break;

	case LC_VIEW_SPLIT_HORIZONTAL:
		SplitHorizontal();
		break;

	case LC_VIEW_SPLIT_VERTICAL:
		SplitVertical();
		break;

	case LC_VIEW_REMOVE_VIEW:
		RemoveActiveView();
		break;

	case LC_VIEW_RESET_VIEWS:
		ResetViews();
		break;

	case LC_VIEW_TOOLBAR_STANDARD:
		ToggleDockWidget(mStandardToolBar);
		break;

	case LC_VIEW_TOOLBAR_TOOLS:
		ToggleDockWidget(mToolsToolBar);
		break;

	case LC_VIEW_TOOLBAR_TIME:
		ToggleDockWidget(mTimeToolBar);
		break;

	case LC_VIEW_TOOLBAR_PARTS:
		ToggleDockWidget(mPartsToolBar);
		break;

	case LC_VIEW_TOOLBAR_COLORS:
		ToggleDockWidget(mColorsToolBar);
		break;

	case LC_VIEW_TOOLBAR_PROPERTIES:
		ToggleDockWidget(mPropertiesToolBar);
		break;

	case LC_VIEW_TOOLBAR_TIMELINE:
		ToggleDockWidget(mTimelineToolBar);
		break;

	case LC_VIEW_TOOLBAR_PREVIEW:
		ToggleDockWidget(mPreviewToolBar);
		break;

	case LC_VIEW_FULLSCREEN:
		ToggleFullScreen();
		break;

	case LC_VIEW_CLOSE_CURRENT_TAB:
		CloseCurrentModelTab();
		break;

	case LC_VIEW_SHADING_WIREFRAME:
		SetShadingMode(lcShadingMode::Wireframe);
		break;

	case LC_VIEW_SHADING_FLAT:
		SetShadingMode(lcShadingMode::Flat);
		break;

	case LC_VIEW_SHADING_DEFAULT_LIGHTS:
		SetShadingMode(lcShadingMode::DefaultLights);
		break;

	case LC_VIEW_PROJECTION_PERSPECTIVE:
		if (ActiveView)
			ActiveView->SetProjection(false);
		break;

	case LC_VIEW_PROJECTION_ORTHO:
		if (ActiveView)
			ActiveView->SetProjection(true);
		break;

	case LC_VIEW_TOGGLE_VIEW_SPHERE:
		ToggleViewSphere();
		break;

	case LC_VIEW_TOGGLE_AXIS_ICON:
		ToggleAxisIcon();
		break;

	case LC_VIEW_TOGGLE_GRID:
		ToggleGrid();
		break;

	case LC_VIEW_FADE_PREVIOUS_STEPS:
		ToggleFadePreviousSteps();
		break;

	case LC_PIECE_INSERT:
		if (ActiveModel)
			ActiveModel->AddPiece();
		break;

	case LC_PIECE_DELETE:
		if (ActiveModel)
			ActiveModel->DeleteSelectedObjects();
		break;

	case LC_PIECE_DUPLICATE:
		if (ActiveModel)
			ActiveModel->DuplicateSelectedPieces();
		break;

    case LC_PIECE_REPLACE:
        if (ActiveModel)
            ActiveModel->ReplaceSelectedPieces();
        break;

    case LC_PIECE_PAINT_SELECTED:
		if (ActiveModel)
			ActiveModel->PaintSelectedPieces();
		break;

	case LC_PIECE_RESET_PIVOT_POINT:
		if (ActiveModel)
			ActiveModel->ResetSelectedPiecesPivotPoint();
		break;

	case LC_PIECE_REMOVE_KEY_FRAMES:
		if (ActiveModel)
			ActiveModel->RemoveSelectedPiecesKeyFrames();
		break;

	case LC_PIECE_CONTROL_POINT_INSERT:
		if (ActiveModel)
			ActiveModel->InsertControlPoint();
		break;

	case LC_PIECE_CONTROL_POINT_REMOVE:
		if (ActiveModel)
			ActiveModel->RemoveFocusedControlPoint();
		break;

	case LC_PIECE_MOVE_PLUSX:
		if (ActiveModel)
			ActiveModel->MoveSelectedObjects(ActiveView->GetMoveDirection(lcVector3(lcMax(GetMoveXYSnap(), 0.1f), 0.0f, 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_MOVE_MINUSX:
		if (ActiveModel)
			ActiveModel->MoveSelectedObjects(ActiveView->GetMoveDirection(lcVector3(-lcMax(GetMoveXYSnap(), 0.1f), 0.0f, 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_MOVE_PLUSY:
		if (ActiveModel)
			ActiveModel->MoveSelectedObjects(ActiveView->GetMoveDirection(lcVector3(0.0f, lcMax(GetMoveXYSnap(), 0.1f), 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_MOVE_MINUSY:
		if (ActiveModel)
			ActiveModel->MoveSelectedObjects(ActiveView->GetMoveDirection(lcVector3(0.0f, -lcMax(GetMoveXYSnap(), 0.1f), 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_MOVE_PLUSZ:
		if (ActiveModel)
			ActiveModel->MoveSelectedObjects(ActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, lcMax(GetMoveZSnap(), 0.1f))), true, false, true, true);
		break;

	case LC_PIECE_MOVE_MINUSZ:
		if (ActiveModel)
			ActiveModel->MoveSelectedObjects(ActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, -lcMax(GetMoveZSnap(), 0.1f))), true, false, true, true);
		break;

	case LC_PIECE_ROTATE_PLUSX:
		if (ActiveModel)
			ActiveModel->RotateSelectedPieces(ActiveView->GetMoveDirection(lcVector3(lcMax(GetAngleSnap(), 1.0f), 0.0f, 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_ROTATE_MINUSX:
		if (ActiveModel)
			ActiveModel->RotateSelectedPieces(ActiveView->GetMoveDirection(-lcVector3(lcMax(GetAngleSnap(), 1.0f), 0.0f, 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_ROTATE_PLUSY:
		if (ActiveModel)
			ActiveModel->RotateSelectedPieces(ActiveView->GetMoveDirection(lcVector3(0.0f, lcMax(GetAngleSnap(), 1.0f), 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_ROTATE_MINUSY:
		if (ActiveModel)
			ActiveModel->RotateSelectedPieces(ActiveView->GetMoveDirection(lcVector3(0.0f, -lcMax(GetAngleSnap(), 1.0f), 0.0f)), true, false, true, true);
		break;

	case LC_PIECE_ROTATE_PLUSZ:
		if (ActiveModel)
			ActiveModel->RotateSelectedPieces(ActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, lcMax(GetAngleSnap(), 1.0f))), true, false, true, true);
		break;

	case LC_PIECE_ROTATE_MINUSZ:
		if (ActiveModel)
			ActiveModel->RotateSelectedPieces(ActiveView->GetMoveDirection(lcVector3(0.0f, 0.0f, -lcMax(GetAngleSnap(), 1.0f))), true, false, true, true);
		break;

	case LC_PIECE_MINIFIG_WIZARD:
		if (ActiveModel)
			ActiveModel->ShowMinifigDialog();
		break;

	case LC_PIECE_ARRAY:
		if (ActiveModel)
			ActiveModel->ShowArrayDialog();
		break;

	case LC_PIECE_VIEW_SELECTED_MODEL:
		SetModelFromSelection();
		break;

	case LC_PIECE_MOVE_SELECTION_TO_MODEL:
		if (ActiveModel)
			ActiveModel->MoveSelectionToModel(lcGetActiveProject()->CreateNewModel(false));
		break;

	case LC_PIECE_INLINE_SELECTED_MODELS:
		if (ActiveModel)
			ActiveModel->InlineSelectedModels();
		break;

	case LC_PIECE_EDIT_END_SUBMODEL:
		if (ActiveView)
		{
			ActiveView->SetTopSubmodelActive();
			if (ActiveModel)
			{
				std::vector<lcModel*> UpdatedModels;
				ActiveModel->UpdatePieceInfo(UpdatedModels);
			}
		}
		break;

	case LC_PIECE_EDIT_SELECTED_SUBMODEL:
		if (ActiveView)
			ActiveView->SetSelectedSubmodelActive();
		break;

	case LC_PIECE_GROUP:
		if (ActiveModel)
			ActiveModel->GroupSelection();
		break;

	case LC_PIECE_UNGROUP:
		if (ActiveModel)
			ActiveModel->UngroupSelection();
		break;

	case LC_PIECE_GROUP_ADD:
		if (ActiveModel)
			ActiveModel->AddSelectedPiecesToGroup();
		break;

	case LC_PIECE_GROUP_REMOVE:
		if (ActiveModel)
			ActiveModel->RemoveFocusPieceFromGroup();
		break;

	case LC_PIECE_GROUP_EDIT:
		if (ActiveModel)
			ActiveModel->ShowEditGroupsDialog();
		break;

	case LC_PIECE_HIDE_SELECTED:
		if (ActiveModel)
			ActiveModel->HideSelectedPieces();
		break;

	case LC_PIECE_HIDE_UNSELECTED:
		if (ActiveModel)
			ActiveModel->HideUnselectedPieces();
		break;

	case LC_PIECE_UNHIDE_SELECTED:
		if (ActiveModel)
			ActiveModel->UnhideSelectedPieces();
		break;

	case LC_PIECE_UNHIDE_ALL:
		if (ActiveModel)
			ActiveModel->UnhideAllPieces();
		break;

	case LC_PIECE_SHOW_EARLIER:
		if (ActiveModel)
			ActiveModel->ShowSelectedPiecesEarlier();
		break;

	case LC_PIECE_SHOW_LATER:
		if (ActiveModel)
			ActiveModel->ShowSelectedPiecesLater();
		break;

	case LC_VIEW_PREFERENCES:
		gApplication->ShowPreferencesDialog();
		break;

	case LC_VIEW_ZOOM_IN:
		if (ActiveView)
			ActiveView->Zoom(10.0f);
		break;

	case LC_VIEW_ZOOM_OUT:
		if (ActiveView)
			ActiveView->Zoom(-10.0f);
		break;

	case LC_VIEW_ZOOM_EXTENTS:
		if (ActiveView)
			ActiveView->ZoomExtents();
		break;

	case LC_VIEW_LOOK_AT:
		if (ActiveView)
			ActiveView->LookAt();
		break;

	case LC_VIEW_MOVE_FORWARD:
		if (ActiveView)
			ActiveView->MoveCamera(lcVector3(0.0f, 0.0f, -1.0f));
		break;

	case LC_VIEW_MOVE_BACKWARD:
		if (ActiveView)
			ActiveView->MoveCamera(lcVector3(0.0f, 0.0f, 1.0f));
		break;

	case LC_VIEW_MOVE_LEFT:
		if (ActiveView)
			ActiveView->MoveCamera(lcVector3(-1.0f, 0.0f, 0.0f));
		break;

	case LC_VIEW_MOVE_RIGHT:
		if (ActiveView)
			ActiveView->MoveCamera(lcVector3(1.0f, 0.0f, 0.0f));
		break;

	case LC_VIEW_MOVE_UP:
		if (ActiveView)
			ActiveView->MoveCamera(lcVector3(0.0f, 1.0f, 0.0f));
		break;

	case LC_VIEW_MOVE_DOWN:
		if (ActiveView)
			ActiveView->MoveCamera(lcVector3(0.0f, -1.0f, 0.0f));
		break;

	case LC_VIEW_TIME_NEXT:
		if (ActiveModel)
			ActiveModel->ShowNextStep();
		break;

	case LC_VIEW_TIME_PREVIOUS:
		if (ActiveModel)
			ActiveModel->ShowPreviousStep();
		break;

	case LC_VIEW_TIME_FIRST:
		if (ActiveModel)
			ActiveModel->ShowFirstStep();
		break;

	case LC_VIEW_TIME_LAST:
		if (ActiveModel)
			ActiveModel->ShowLastStep();
		break;

	case LC_VIEW_TIME_INSERT_BEFORE:
		lcGetActiveModel()->InsertStep(lcGetActiveModel()->GetCurrentStep());
		break;

	case LC_VIEW_TIME_INSERT_AFTER:
		lcGetActiveModel()->InsertStep(lcGetActiveModel()->GetCurrentStep() + 1);
		break;

	case LC_VIEW_TIME_DELETE:
		lcGetActiveModel()->RemoveStep(lcGetActiveModel()->GetCurrentStep());
		break;

	case LC_VIEW_VIEWPOINT_FRONT:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Front);
		break;

	case LC_VIEW_VIEWPOINT_BACK:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Back);
		break;

	case LC_VIEW_VIEWPOINT_TOP:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Top);
		break;

	case LC_VIEW_VIEWPOINT_BOTTOM:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Bottom);
		break;

	case LC_VIEW_VIEWPOINT_LEFT:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Left);
		break;

	case LC_VIEW_VIEWPOINT_RIGHT:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Right);
		break;

	case LC_VIEW_VIEWPOINT_HOME:
		if (ActiveView)
			ActiveView->SetViewpoint(lcViewpoint::Home);
		break;

	case LC_VIEW_CAMERA_NONE:
		if (ActiveView)
			ActiveView->RemoveCamera();
		break;

	case LC_VIEW_CAMERA1:
	case LC_VIEW_CAMERA2:
	case LC_VIEW_CAMERA3:
	case LC_VIEW_CAMERA4:
	case LC_VIEW_CAMERA5:
	case LC_VIEW_CAMERA6:
	case LC_VIEW_CAMERA7:
	case LC_VIEW_CAMERA8:
	case LC_VIEW_CAMERA9:
	case LC_VIEW_CAMERA10:
	case LC_VIEW_CAMERA11:
	case LC_VIEW_CAMERA12:
	case LC_VIEW_CAMERA13:
	case LC_VIEW_CAMERA14:
	case LC_VIEW_CAMERA15:
	case LC_VIEW_CAMERA16:
		if (ActiveView)
			ActiveView->SetCameraIndex(CommandId - LC_VIEW_CAMERA1);
		break;

	case LC_MODEL_NEW:
		lcGetActiveProject()->CreateNewModel(true);
		break;

	case LC_MODEL_PROPERTIES:
		lcGetActiveModel()->ShowPropertiesDialog();
		break;

	case LC_MODEL_LIST:
		lcGetActiveProject()->ShowModelListDialog();
		break;

	case LC_MODEL_01:
	case LC_MODEL_02:
	case LC_MODEL_03:
	case LC_MODEL_04:
	case LC_MODEL_05:
	case LC_MODEL_06:
	case LC_MODEL_07:
	case LC_MODEL_08:
	case LC_MODEL_09:
	case LC_MODEL_10:
	case LC_MODEL_11:
	case LC_MODEL_12:
	case LC_MODEL_13:
	case LC_MODEL_14:
	case LC_MODEL_15:
	case LC_MODEL_16:
	case LC_MODEL_17:
	case LC_MODEL_18:
	case LC_MODEL_19:
	case LC_MODEL_20:
	case LC_MODEL_21:
	case LC_MODEL_22:
	case LC_MODEL_23:
	case LC_MODEL_24:
	case LC_MODEL_25:
	case LC_MODEL_26:
	case LC_MODEL_27:
	case LC_MODEL_28:
	case LC_MODEL_29:
	case LC_MODEL_30:
	case LC_MODEL_31:
	case LC_MODEL_32:
	case LC_MODEL_33:
	case LC_MODEL_34:
	case LC_MODEL_35:
	case LC_MODEL_36:
	case LC_MODEL_37:
	case LC_MODEL_38:
	case LC_MODEL_39:
	case LC_MODEL_40:
		lcGetActiveProject()->SetActiveModel(CommandId - LC_MODEL_01);
		break;

	case LC_HELP_HOMEPAGE:
		QDesktopServices::openUrl(QUrl("https://www.leocad.org/"));
		break;

	case LC_HELP_BUG_REPORT:
		QDesktopServices::openUrl(QUrl("https://github.com/leozide/leocad/issues"));
		break;

	case LC_HELP_UPDATES:
		ShowUpdatesDialog();
		break;

	case LC_HELP_ABOUT:
		ShowAboutDialog();
		break;

	case LC_VIEW_TIME_ADD_KEYS:
		SetAddKeys(!GetAddKeys());
		break;

	case LC_EDIT_TRANSFORM_RELATIVE:
		SetRelativeTransform(true);
		break;

	case LC_EDIT_TRANSFORM_ABSOLUTE:
		SetRelativeTransform(false);
		break;

	case LC_EDIT_TRANSFORM_TOGGLE_RELATIVE:
		SetRelativeTransform(!GetRelativeTransform());
		break;

	case LC_EDIT_TRANSFORM_SEPARATELY:
		SetSeparateTransform(true);
		break;

	case LC_EDIT_TRANSFORM_TOGETHER:
		SetSeparateTransform(false);
		break;

	case LC_EDIT_TRANSFORM_TOGGLE_SEPARATE:
		SetSeparateTransform(!GetSeparateTransform());
		break;

	case LC_EDIT_SNAP_MOVE_TOGGLE:
		SetMoveSnapEnabled(!mMoveSnapEnabled);
		break;

	case LC_EDIT_SNAP_MOVE_XY0:
	case LC_EDIT_SNAP_MOVE_XY1:
	case LC_EDIT_SNAP_MOVE_XY2:
	case LC_EDIT_SNAP_MOVE_XY3:
	case LC_EDIT_SNAP_MOVE_XY4:
	case LC_EDIT_SNAP_MOVE_XY5:
	case LC_EDIT_SNAP_MOVE_XY6:
	case LC_EDIT_SNAP_MOVE_XY7:
	case LC_EDIT_SNAP_MOVE_XY8:
	case LC_EDIT_SNAP_MOVE_XY9:
		SetMoveXYSnapIndex(CommandId - LC_EDIT_SNAP_MOVE_XY0);
		break;

	case LC_EDIT_SNAP_MOVE_Z0:
	case LC_EDIT_SNAP_MOVE_Z1:
	case LC_EDIT_SNAP_MOVE_Z2:
	case LC_EDIT_SNAP_MOVE_Z3:
	case LC_EDIT_SNAP_MOVE_Z4:
	case LC_EDIT_SNAP_MOVE_Z5:
	case LC_EDIT_SNAP_MOVE_Z6:
	case LC_EDIT_SNAP_MOVE_Z7:
	case LC_EDIT_SNAP_MOVE_Z8:
	case LC_EDIT_SNAP_MOVE_Z9:
		SetMoveZSnapIndex(CommandId - LC_EDIT_SNAP_MOVE_Z0);
		break;

	case LC_EDIT_SNAP_ANGLE_TOGGLE:
		SetAngleSnapEnabled(!mAngleSnapEnabled);
		break;

	case LC_EDIT_SNAP_ANGLE0:
	case LC_EDIT_SNAP_ANGLE1:
	case LC_EDIT_SNAP_ANGLE2:
	case LC_EDIT_SNAP_ANGLE3:
	case LC_EDIT_SNAP_ANGLE4:
	case LC_EDIT_SNAP_ANGLE5:
	case LC_EDIT_SNAP_ANGLE6:
	case LC_EDIT_SNAP_ANGLE7:
	case LC_EDIT_SNAP_ANGLE8:
	case LC_EDIT_SNAP_ANGLE9:
		SetAngleSnapIndex(CommandId - LC_EDIT_SNAP_ANGLE0);
		break;

	case LC_EDIT_TRANSFORM:
		if (ActiveModel)
			ActiveModel->TransformSelectedObjects(GetTransformType(), GetTransformAmount());
		break;

	case LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION:
		SetTransformType(lcTransformType::AbsoluteTranslation);
		break;

	case LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION:
		SetTransformType(lcTransformType::RelativeTranslation);
		break;

	case LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION:
		SetTransformType(lcTransformType::AbsoluteRotation);
		break;

	case LC_EDIT_TRANSFORM_RELATIVE_ROTATION:
		SetTransformType(lcTransformType::RelativeRotation);
		break;

	case LC_EDIT_ACTION_SELECT:
		SetTool(lcTool::Select);
		break;

	case LC_EDIT_ACTION_INSERT:
		SetTool(lcTool::Insert);
		break;

	case LC_EDIT_ACTION_LIGHT:
		SetTool(lcTool::Light);
		break;

	case LC_EDIT_ACTION_SPOTLIGHT:
		SetTool(lcTool::SpotLight);
		break;

	case LC_EDIT_ACTION_CAMERA:
		SetTool(lcTool::Camera);
		break;

	case LC_EDIT_ACTION_MOVE:
		SetTool(lcTool::Move);
		break;

	case LC_EDIT_ACTION_ROTATE:
		SetTool(lcTool::Rotate);
		break;

	case LC_EDIT_ACTION_DELETE:
		SetTool(lcTool::Eraser);
		break;

	case LC_EDIT_ACTION_PAINT:
		SetTool(lcTool::Paint);
		break;

	case LC_EDIT_ACTION_COLOR_PICKER:
		SetTool(lcTool::ColorPicker);
		break;

	case LC_EDIT_ACTION_ZOOM:
		SetTool(lcTool::Zoom);
		break;

	case LC_EDIT_ACTION_ZOOM_REGION:
		SetTool(lcTool::ZoomRegion);
		break;

	case LC_EDIT_ACTION_PAN:
		SetTool(lcTool::Pan);
		break;

	case LC_EDIT_ACTION_ROTATE_VIEW:
		SetTool(lcTool::RotateView);
		break;

	case LC_EDIT_ACTION_ROLL:
		SetTool(lcTool::Roll);
		break;

	case LC_EDIT_CANCEL:
		if (ActiveView && !ActiveView->CloseFindReplaceDialog())
			ActiveView->CancelTrackingOrClearSelection();
		break;

	case LC_TIMELINE_INSERT_BEFORE:
		mTimelineWidget->InsertStepBefore();
		break;

	case LC_TIMELINE_INSERT_AFTER:
		mTimelineWidget->InsertStepAfter();
		break;

	case LC_TIMELINE_DELETE:
		mTimelineWidget->RemoveStep();
		break;

	case LC_TIMELINE_MOVE_SELECTION:
		mTimelineWidget->MoveSelection();
		break;

	case LC_TIMELINE_MOVE_SELECTION_BEFORE:
		mTimelineWidget->MoveSelectionBefore();
		break;

	case LC_TIMELINE_MOVE_SELECTION_AFTER:
		mTimelineWidget->MoveSelectionAfter();
		break;

	case LC_TIMELINE_SET_CURRENT:
		mTimelineWidget->SetCurrentStep();
		break;

	case LC_NUM_COMMANDS:
		break;
	}
}
