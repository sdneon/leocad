#ifndef _LC_TIMELINEWIDGET_H_
#define _LC_TIMELINEWIDGET_H_

class lcTimelineWidget : public QTreeWidget
{
	Q_OBJECT

public:
	lcTimelineWidget(QWidget* Parent);
	virtual ~lcTimelineWidget();

	void Update(bool Clear, bool UpdateItems);
	void UpdateSelection();

	void InsertStep();
	void RemoveStep();
	void MoveSelection();
	void SetCurrentStep();

public slots:
	void ItemSelectionChanged();
	void CustomMenuRequested(QPoint Pos);

protected:
	virtual void dropEvent(QDropEvent* Event);
	virtual void mousePressEvent(QMouseEvent* Event);
	void UpdateModel();

	QMap<int, QIcon> mIcons;
	QMap<lcPiece*, QTreeWidgetItem*> mItems;
	bool mIgnoreUpdates;
};

#endif // _LC_TIMELINEWIDGET_H_
