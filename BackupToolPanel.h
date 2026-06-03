#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QProgressBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QComboBox>
#include <QTimer>
#include <QStringList>
#include <QScrollArea>
#include <QMap>

class BackupToolPanel : public QWidget
{
	Q_OBJECT

public:
	BackupToolPanel(const QString& gamePath, QWidget* parent = nullptr);

public slots:
	void refreshSourceItems();

private slots:
	void onBrowseDestination();
	void onSelectAll();
	void onClearAll();
	void onBackupNow();
	void onStartAutoBackup();
	void onAutoBackupTick();
	void processNextChunk();

private:
	void buildUi();
	void populateCheckboxes();
	void queueFilesForBackup();
	void onBackupComplete();
	void updateSnapshot();
	void addHistoryEntry(const QString& type, qint64 fileCount, qint64 totalSize);
	void updateChangesList();
	void loadHistory();
	void loadSettings();
	void saveSettings();
	void setBackupUiEnabled(bool enabled);
	static QString fileSignature(const QString& filePath);

	QString mGamePath;
	QString mDestinationPath;

	QLineEdit* mDestinationEdit;
	QPushButton* mBrowseButton;
	QScrollArea* mCheckboxScroll;
	QWidget* mCheckboxContainer;
	QVBoxLayout* mCheckboxLayout;
	QPushButton* mSelectAllButton;
	QPushButton* mClearAllButton;
	QPushButton* mBackupNowButton;
	QCheckBox* mChangedOnlyCheck;
	QComboBox* mAutoIntervalCombo;
	QPushButton* mStartAutoButton;
	QProgressBar* mProgressBar;
	QTabWidget* mTabWidget;
	QTableWidget* mHistoryTable;
	QListWidget* mChangesList;
	QTimer* mAutoTimer;

	QList<QCheckBox*> mItemCheckboxes;
	QStringList mItemNames;

	QStringList mPendingFileQueue;
	QMap<QString, QString> mSnapshotUpdate;
	int mPendingQueueIndex;
	qint64 mTotalFilesToCopy;
	qint64 mTotalBytesToCopy;
	bool mBackupRunning;

	QList<QVariantMap> mHistoryEntries;
	QStringList mLastChanges;
};
