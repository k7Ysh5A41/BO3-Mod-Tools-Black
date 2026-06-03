#include "stdafx.h"
#include "BackupToolPanel.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QLabel>

BackupToolPanel::BackupToolPanel(const QString& gamePath, QWidget* parent)
	: QWidget(parent)
	, mGamePath(QDir::cleanPath(gamePath))
	, mAutoTimer(nullptr)
	, mPendingQueueIndex(0)
	, mTotalFilesToCopy(0)
	, mTotalBytesToCopy(0)
	, mBackupRunning(false)
{
	buildUi();
	loadSettings();
	loadHistory();
	populateCheckboxes();
}

void BackupToolPanel::buildUi()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(6);

	// Destination folder row
	QHBoxLayout* destLayout = new QHBoxLayout();
	QLabel* destLabel = new QLabel("Destination:", this);
	mDestinationEdit = new QLineEdit(this);
	mDestinationEdit->setPlaceholderText("Select backup destination folder...");
	mBrowseButton = new QPushButton("Browse", this);
	destLayout->addWidget(destLabel);
	destLayout->addWidget(mDestinationEdit, 1);
	destLayout->addWidget(mBrowseButton);
	mainLayout->addLayout(destLayout);

	// Source items label
	QLabel* sourceLabel = new QLabel("Source items to back up:", this);
	mainLayout->addWidget(sourceLabel);

	// Source items checkboxes in a scroll area
	mCheckboxScroll = new QScrollArea(this);
	mCheckboxScroll->setWidgetResizable(true);
	mCheckboxScroll->setFrameShape(QFrame::NoFrame);
	mCheckboxContainer = new QWidget(mCheckboxScroll);
	mCheckboxLayout = new QVBoxLayout(mCheckboxContainer);
	mCheckboxLayout->setContentsMargins(0, 0, 0, 0);
	mCheckboxLayout->setSpacing(2);
	mCheckboxScroll->setWidget(mCheckboxContainer);
	mCheckboxScroll->setMinimumHeight(120);
	mCheckboxScroll->setMaximumHeight(200);
	mainLayout->addWidget(mCheckboxScroll);

	// Select All / Clear All row
	QHBoxLayout* selectLayout = new QHBoxLayout();
	mSelectAllButton = new QPushButton("Select All", this);
	mClearAllButton = new QPushButton("Clear All", this);
	selectLayout->addWidget(mSelectAllButton);
	selectLayout->addWidget(mClearAllButton);
	selectLayout->addStretch();
	mainLayout->addLayout(selectLayout);

	// Changed Files Only + auto-backup interval + Start Auto
	QHBoxLayout* autoLayout = new QHBoxLayout();
	mChangedOnlyCheck = new QCheckBox("Changed Files Only", this);
	mAutoIntervalCombo = new QComboBox(this);
	mAutoIntervalCombo->addItem("Disabled", 0);
	mAutoIntervalCombo->addItem("5 minutes", 5 * 60 * 1000);
	mAutoIntervalCombo->addItem("10 minutes", 10 * 60 * 1000);
	mAutoIntervalCombo->addItem("30 minutes", 30 * 60 * 1000);
	mAutoIntervalCombo->addItem("1 hour", 60 * 60 * 1000);
	mAutoIntervalCombo->addItem("2 hours", 2 * 60 * 60 * 1000);
	mStartAutoButton = new QPushButton("Start Auto", this);
	autoLayout->addWidget(mChangedOnlyCheck);
	autoLayout->addWidget(mAutoIntervalCombo);
	autoLayout->addWidget(mStartAutoButton);
	mainLayout->addLayout(autoLayout);

	// Backup Now button
	mBackupNowButton = new QPushButton("Backup Now", this);
	mainLayout->addWidget(mBackupNowButton);

	// Progress bar
	mProgressBar = new QProgressBar(this);
	mProgressBar->setRange(0, 100);
	mProgressBar->setValue(0);
	mainLayout->addWidget(mProgressBar);

	// Tab widget with History and Changes
	mTabWidget = new QTabWidget(this);
	mHistoryTable = new QTableWidget(0, 5, mTabWidget);
	mHistoryTable->setHorizontalHeaderLabels(QStringList() << "Date" << "Type" << "Files" << "Size" << "Path");
	mHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	mHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mHistoryTable->horizontalHeader()->setStretchLastSection(true);
	mHistoryTable->verticalHeader()->setVisible(false);
	mTabWidget->addTab(mHistoryTable, "History");

	mChangesList = new QListWidget(mTabWidget);
	mTabWidget->addTab(mChangesList, "Changes");
	mainLayout->addWidget(mTabWidget, 1);

	// Connections
	connect(mBrowseButton, &QPushButton::clicked, this, &BackupToolPanel::onBrowseDestination);
	connect(mSelectAllButton, &QPushButton::clicked, this, &BackupToolPanel::onSelectAll);
	connect(mClearAllButton, &QPushButton::clicked, this, &BackupToolPanel::onClearAll);
	connect(mBackupNowButton, &QPushButton::clicked, this, &BackupToolPanel::onBackupNow);
	connect(mStartAutoButton, &QPushButton::clicked, this, &BackupToolPanel::onStartAutoBackup);
}

void BackupToolPanel::populateCheckboxes()
{
	for (QCheckBox* cb : mItemCheckboxes)
	{
		mCheckboxLayout->removeWidget(cb);
		delete cb;
	}
	mItemCheckboxes.clear();
	mItemNames.clear();

	if (mGamePath.isEmpty())
		return;

	QDir gameDir(mGamePath);
	if (!gameDir.exists())
		return;

	QFileInfoList entries = gameDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo& entry : entries)
	{
		QString name = entry.fileName();
		QCheckBox* cb = new QCheckBox(name, mCheckboxContainer);
		cb->setChecked(true);
		mCheckboxLayout->addWidget(cb);
		mItemCheckboxes.append(cb);
		mItemNames.append(name);
	}

	mCheckboxLayout->addStretch();
}

void BackupToolPanel::refreshSourceItems()
{
	QSet<QString> checkedItems;
	for (int i = 0; i < mItemCheckboxes.size(); i++)
	{
		if (mItemCheckboxes[i]->isChecked())
			checkedItems.insert(mItemNames[i]);
	}

	for (QCheckBox* cb : mItemCheckboxes)
	{
		mCheckboxLayout->removeWidget(cb);
		delete cb;
	}
	mItemCheckboxes.clear();
	mItemNames.clear();

	// Remove the trailing stretch if any
	QLayoutItem* stretchItem = mCheckboxLayout->takeAt(mCheckboxLayout->count() - 1);
	if (stretchItem)
	{
		delete stretchItem;
	}

	populateCheckboxes();

	for (int i = 0; i < mItemNames.size(); i++)
	{
		if (checkedItems.contains(mItemNames[i]))
			mItemCheckboxes[i]->setChecked(true);
	}
}

void BackupToolPanel::onBrowseDestination()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Destination", mDestinationEdit->text());
	if (!dir.isEmpty())
	{
		mDestinationEdit->setText(QDir::toNativeSeparators(dir));
		mDestinationPath = QDir::cleanPath(dir);
		saveSettings();
	}
}

void BackupToolPanel::onSelectAll()
{
	for (QCheckBox* cb : mItemCheckboxes)
		cb->setChecked(true);
}

void BackupToolPanel::onClearAll()
{
	for (QCheckBox* cb : mItemCheckboxes)
		cb->setChecked(false);
}

void BackupToolPanel::onStartAutoBackup()
{
	if (mAutoTimer && mAutoTimer->isActive())
	{
		mAutoTimer->stop();
		mStartAutoButton->setText("Start Auto");
		return;
	}

	int intervalMs = mAutoIntervalCombo->currentData().toInt();
	if (intervalMs <= 0)
	{
		QMessageBox::information(this, "Auto Backup", "Select a backup interval first.");
		return;
	}

	if (!mAutoTimer)
	{
		mAutoTimer = new QTimer(this);
		connect(mAutoTimer, &QTimer::timeout, this, &BackupToolPanel::onAutoBackupTick);
	}

	mAutoTimer->start(intervalMs);
	mStartAutoButton->setText("Stop Auto");
}

void BackupToolPanel::onAutoBackupTick()
{
	if (mBackupRunning || mDestinationPath.isEmpty())
		return;

	onBackupNow();
}

void BackupToolPanel::onBackupNow()
{
	if (mBackupRunning)
		return;

	if (mDestinationPath.isEmpty())
	{
		mDestinationPath = QDir::cleanPath(mDestinationEdit->text());
		if (mDestinationPath.isEmpty())
		{
			QMessageBox::warning(this, "Backup", "Please select a destination folder first.");
			return;
		}
	}

	bool anyChecked = false;
	for (QCheckBox* cb : mItemCheckboxes)
	{
		if (cb->isChecked())
		{
			anyChecked = true;
			break;
		}
	}
	if (!anyChecked)
	{
		QMessageBox::warning(this, "Backup", "No source items selected for backup.");
		return;
	}

	setBackupUiEnabled(false);
	mBackupRunning = true;
	mProgressBar->setValue(0);

	queueFilesForBackup();

	if (mPendingFileQueue.isEmpty())
	{
		QMessageBox::information(this, "Backup", "No files need to be backed up.");
		setBackupUiEnabled(true);
		mBackupRunning = false;
		return;
	}

	processNextChunk();
}

void BackupToolPanel::queueFilesForBackup()
{
	mPendingFileQueue.clear();
	mSnapshotUpdate.clear();
	mPendingQueueIndex = 0;
	mTotalFilesToCopy = 0;
	mTotalBytesToCopy = 0;

	QSettings settings;

	for (int i = 0; i < mItemCheckboxes.size(); i++)
	{
		if (!mItemCheckboxes[i]->isChecked())
			continue;

		QString srcRoot = QDir::cleanPath(mGamePath + "/" + mItemNames[i]);
		QFileInfo srcInfo(srcRoot);
		if (!srcInfo.exists())
			continue;

		if (srcInfo.isFile())
		{
			QString relPath = mItemNames[i];
			QString sig = fileSignature(srcRoot);
			mSnapshotUpdate.insert(relPath, sig);

			if (mChangedOnlyCheck->isChecked())
			{
				QString destPath = QDir::cleanPath(mDestinationPath + "/" + relPath);
				QString storedSig = settings.value("BackupTool/Snapshot/" + relPath).toString();
				if (storedSig == sig && QFileInfo::exists(destPath))
					continue;
			}

			mPendingFileQueue.append(relPath);
			mTotalBytesToCopy += srcInfo.size();
		}
		else if (srcInfo.isDir())
		{
			QDirIterator it(srcRoot, QDir::Files | QDir::NoSymLinks | QDir::Hidden, QDirIterator::Subdirectories);
			while (it.hasNext())
			{
				it.next();
				QString absPath = it.filePath();
				QString relPath = absPath.mid(mGamePath.length() + 1);
				QString sig = fileSignature(absPath);
				mSnapshotUpdate.insert(relPath, sig);

				if (mChangedOnlyCheck->isChecked())
				{
					QString destPath = QDir::cleanPath(mDestinationPath + "/" + relPath);
					QString storedSig = settings.value("BackupTool/Snapshot/" + relPath).toString();
					if (storedSig == sig && QFileInfo::exists(destPath))
						continue;
				}

				mPendingFileQueue.append(relPath);
				mTotalBytesToCopy += it.fileInfo().size();
			}
		}
	}

	mTotalFilesToCopy = mPendingFileQueue.size();
	mProgressBar->setMaximum(mTotalFilesToCopy > 0 ? mTotalFilesToCopy : 1);
	mProgressBar->setValue(0);
}

void BackupToolPanel::processNextChunk()
{
	QStringList changedFiles;
	int processed = 0;

	while (mPendingQueueIndex < mPendingFileQueue.size() && processed < 20)
	{
		QString relPath = mPendingFileQueue[mPendingQueueIndex];
		QString srcPath = QDir::cleanPath(mGamePath + "/" + relPath);
		QString destPath = QDir::cleanPath(mDestinationPath + "/" + relPath);

		QDir().mkpath(QFileInfo(destPath).absolutePath());

		if (QFile::exists(destPath))
			QFile::remove(destPath);

		if (QFile::copy(srcPath, destPath))
			changedFiles.append(relPath);

		mPendingQueueIndex++;
		processed++;
	}

	mProgressBar->setValue(mPendingQueueIndex);
	mLastChanges.append(changedFiles);

	if (mPendingQueueIndex >= mPendingFileQueue.size())
	{
		onBackupComplete();
	}
	else
	{
		QTimer::singleShot(0, this, &BackupToolPanel::processNextChunk);
	}
}

void BackupToolPanel::onBackupComplete()
{
	QString backupType = mChangedOnlyCheck->isChecked() ? "Incremental" : "Full";

	qint64 totalSize = 0;
	for (const QString& relPath : mPendingFileQueue)
	{
		QString destPath = QDir::cleanPath(mDestinationPath + "/" + relPath);
		QFileInfo fi(destPath);
		if (fi.exists())
			totalSize += fi.size();
	}

	updateSnapshot();
	addHistoryEntry(backupType, mPendingFileQueue.size(), totalSize);
	updateChangesList();
	saveSettings();

	setBackupUiEnabled(true);
	mBackupRunning = false;

	if (mPendingFileQueue.size() > 0)
	{
		QMessageBox::information(this, "Backup Complete",
			QString("Backed up %1 files (%2).")
				.arg(mPendingFileQueue.size())
				.arg(QLocale().formattedDataSize(totalSize)));
	}
}

void BackupToolPanel::updateSnapshot()
{
	QSettings settings;
	for (auto it = mSnapshotUpdate.begin(); it != mSnapshotUpdate.end(); ++it)
		settings.setValue("BackupTool/Snapshot/" + it.key(), it.value());
}

void BackupToolPanel::addHistoryEntry(const QString& type, qint64 fileCount, qint64 totalSize)
{
	QVariantMap entry;
	entry["date"] = QDateTime::currentDateTime().toString(Qt::ISODate);
	entry["type"] = type;
	entry["files"] = fileCount;
	entry["size"] = totalSize;
	entry["path"] = mDestinationPath;

	mHistoryEntries.prepend(entry);

	QSettings settings;
	QVariantList historyList;
	for (const QVariantMap& e : mHistoryEntries)
		historyList.append(e);
	settings.setValue("BackupTool/History", historyList);

	loadHistory();
}

void BackupToolPanel::updateChangesList()
{
	mChangesList->clear();
	for (const QString& filePath : mLastChanges)
		mChangesList->addItem(filePath);

	if (mChangesList->count() == 0)
		mChangesList->addItem("No files changed in last backup.");

	QSettings settings;
	settings.setValue("BackupTool/LastChanges", mLastChanges);
}

void BackupToolPanel::loadHistory()
{
	QSettings settings;
	QVariantList historyList = settings.value("BackupTool/History").toList();
	mHistoryEntries.clear();
	for (const QVariant& v : historyList)
		mHistoryEntries.append(v.toMap());

	mHistoryTable->setRowCount(mHistoryEntries.size());
	for (int row = 0; row < mHistoryEntries.size(); row++)
	{
		const QVariantMap& e = mHistoryEntries[row];
		mHistoryTable->setItem(row, 0, new QTableWidgetItem(e.value("date").toString()));
		mHistoryTable->setItem(row, 1, new QTableWidgetItem(e.value("type").toString()));

		qint64 fileCount = e.value("files").toLongLong();
		qint64 sizeBytes = e.value("size").toLongLong();
		mHistoryTable->setItem(row, 2, new QTableWidgetItem(QString::number(fileCount)));
		mHistoryTable->setItem(row, 3, new QTableWidgetItem(QLocale().formattedDataSize(sizeBytes)));
		mHistoryTable->setItem(row, 4, new QTableWidgetItem(e.value("path").toString()));
	}
	mHistoryTable->resizeColumnsToContents();

	mLastChanges = settings.value("BackupTool/LastChanges").toStringList();
	updateChangesList();
}

void BackupToolPanel::loadSettings()
{
	QSettings settings;
	settings.beginGroup("BackupTool");

	mDestinationPath = settings.value("DestPath").toString();
	mDestinationEdit->setText(QDir::toNativeSeparators(mDestinationPath));

	mChangedOnlyCheck->setChecked(settings.value("ChangedOnly", false).toBool());

	int intervalIndex = settings.value("AutoInterval", 0).toInt();
	if (intervalIndex >= 0 && intervalIndex < mAutoIntervalCombo->count())
		mAutoIntervalCombo->setCurrentIndex(intervalIndex);

	settings.endGroup();
}

void BackupToolPanel::saveSettings()
{
	QSettings settings;
	settings.beginGroup("BackupTool");

	settings.setValue("DestPath", mDestinationPath);
	settings.setValue("ChangedOnly", mChangedOnlyCheck->isChecked());
	settings.setValue("AutoInterval", mAutoIntervalCombo->currentIndex());

	settings.endGroup();
}

void BackupToolPanel::setBackupUiEnabled(bool enabled)
{
	mBackupNowButton->setEnabled(enabled);
	mBrowseButton->setEnabled(enabled);
	mSelectAllButton->setEnabled(enabled);
	mClearAllButton->setEnabled(enabled);
	mChangedOnlyCheck->setEnabled(enabled);
	mAutoIntervalCombo->setEnabled(enabled);
	if (!enabled && mAutoTimer && mAutoTimer->isActive())
		mStartAutoButton->setEnabled(true);
	else
		mStartAutoButton->setEnabled(enabled);
	mDestinationEdit->setEnabled(enabled);
}

QString BackupToolPanel::fileSignature(const QString& filePath)
{
	QFileInfo fi(filePath);
	return QString("%1|%2").arg(fi.size()).arg(fi.lastModified().toMSecsSinceEpoch());
}
