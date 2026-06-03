#pragma once

#include <QString>
#include <QJsonObject>
#include <QHash>
#include <QStringList>

class LocalizationManager
{
public:
	static LocalizationManager& Instance();

	bool LoadLanguage(const QString& language);
	void LoadAllLanguages(const QStringList& languages);
	QString tr(const QString& key) const;
	QString tr(const QString& key, const QString& defaultValue) const;

	QString currentLanguage() const { return mCurrentLanguage; }
	QStringList availableLanguages() const { return mLanguages; }
	QStringList availableLanguageDisplayNames() const;

	bool SaveTranslation(const QString& language, const QString& key, const QString& value);
	bool SaveTranslationFile(const QString& language);
	QJsonObject GetTranslations(const QString& language) const;
	QString GetEnglishReference(const QString& key) const;

	QString localizationPath() const { return mLocalizationPath; }
	void setLocalizationPath(const QString& path) { mLocalizationPath = path; }
	int translationCount() const { return mTranslations.value(mCurrentLanguage).size(); }

private:
	LocalizationManager() = default;
	~LocalizationManager() = default;
	LocalizationManager(const LocalizationManager&) = delete;
	LocalizationManager& operator=(const LocalizationManager&) = delete;

	QHash<QString, QHash<QString, QString>> mTranslations;
	QHash<QString, QString> mEnglishTranslations;
	QStringList mLanguages;
	QString mCurrentLanguage;
	QString mLocalizationPath;
};
